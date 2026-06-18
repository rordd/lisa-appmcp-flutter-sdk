#!/usr/bin/env bash
# Manual install of com.webos.app.familyboard to a rooted dev webOS TV.
#
# Why manual (not `ares-install`):
#   ares-install rejects appIds starting with com.webos.* / com.lge.* /
#   com.palm.* in dev mode ("Cannot install privileged app on developer
#   mode"). familyboard keeps the com.webos.app.* id to match its appMCP.json
#   / Linux deployment, so we unpack the IPK straight into SAM's store path.
#
# Steps:
#   1. Extract IPK locally (ar + tar)
#   2. Push data tarball to TV -> unpack into /media/cryptofs/apps/
#      (SAM "store" path; persists across reboot, scanned by appmcp-server
#       via --apps-dir /media/cryptofs/apps/usr/palm/applications)
#   3. Deploy LS2 role/client-perms/manifest to /var/luna-service2 + HUP
#      ls-hubd. REQUIRED: a manual tar-copy bypasses appinstalld, which would
#      otherwise auto-generate the app's LS2 role. Without it the webOS Flutter
#      embedder's WebOSServiceBridge fails to register and the app aborts
#      (SIGABRT) at startup, before any Dart runs. Do NOT remove this step.
#   4. Close any running instance (luna-send closeByAppId + hard kill)
#   5. Launch the app (ares-launch for a registered alias, else luna-send over
#      root ssh; appmcp-server also launches it on-demand)
#
# After installing, RESTART the TV's Lisa daemon / appmcp-server so it
# rescans the apps dir and registers familyboard's 3 tools.
#
# Usage: scripts/install-tv.sh [tv-alias]
#   tv-alias defaults to `tv_a2ui`; only its IP is read from the ares entry.
#   Install goes over the TV's root sshd (root@<ip>:22) — the proven path,
#   independent of the ares dev account (prisoner@...:9922). Override with
#   SSH_USER (default: root) / SSH_PORT (default: 22) if needed.

set -euo pipefail

TV_ALIAS="${1:-tv_a2ui}"
APP_ID="com.webos.app.familyboard"
APP_DIR="$(cd "$(dirname "$0")/.." && pwd)"
IPK="$APP_DIR/build/webos/arm/release/ipk/${APP_ID}.ipk"
PERMS_DIR="$APP_DIR/scripts/luna-perms"

if [ ! -f "$IPK" ]; then
  echo "IPK not found: $IPK" >&2
  echo "Build first:  cd $APP_DIR && unset LD_LIBRARY_PATH && \\" >&2
  echo "  flutter-webos build webos --release --no-tree-shake-icons --ipk" >&2
  exit 1
fi

# Resolve the TV's IP (and ssh port). If the arg is already an IP (or TV_HOST
# is set), use it directly — no ares registration needed. Otherwise look it up
# by ares alias (e.g. root@10.157.70.84:2293 → host 10.157.70.84, port 2293),
# which also covers a port-forwarded demo fleet sharing one public IP.
ARES_PORT=""
if [ -n "${TV_HOST:-}" ]; then
  HOST="$TV_HOST"
elif printf '%s' "$TV_ALIAS" | grep -qE '^[0-9]+(\.[0-9]+){3}$'; then
  HOST="$TV_ALIAS"
else
  DEV_INFO="$(ares-setup-device --list 2>/dev/null \
    | awk -v alias="$TV_ALIAS" '$1==alias { print $2 }')"
  if [ -z "$DEV_INFO" ]; then
    echo "Could not find ares device '$TV_ALIAS'. Registered devices:" >&2
    ares-setup-device --list >&2
    echo "Tip: pass the IP directly, e.g. $0 192.168.0.37" >&2
    exit 1
  fi
  HOST="${DEV_INFO#*@}"; HOST="${HOST%%:*}"
  ARES_USER="${DEV_INFO%%@*}"
  # Only adopt the ares port when its user is root — then that port IS the root
  # sshd (e.g. a port-forwarded demo fleet, root@IP:2293). For a dev account
  # like prisoner@IP:9922 the port is the dev port, not root's, so ignore it.
  case "$DEV_INFO" in *:*) [ "$ARES_USER" = root ] && ARES_PORT="${DEV_INFO##*:}" ;; esac
fi

# Manual install needs root (write /media/cryptofs/apps, run luna-send). The
# install always goes over the TV's root sshd. Port precedence: explicit
# SSH_PORT env > root-port parsed from the ares alias > 22.
SSH_USER="${SSH_USER:-root}"
if [ -n "${SSH_PORT:-}" ]; then :; elif [ -n "$ARES_PORT" ]; then SSH_PORT="$ARES_PORT"; else SSH_PORT=22; fi
SSH_TARGET="$SSH_USER@$HOST"
SSH="ssh -o ConnectTimeout=8 -p $SSH_PORT"
SCP="scp -q -P $SSH_PORT"

echo "Target: $SSH_TARGET (port $SSH_PORT)  app: $APP_ID"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

echo "[1/5] Extracting IPK"
cp "$IPK" "$WORK/app.ipk"
( cd "$WORK" && ar x app.ipk )

echo "[2/5] Pushing app payload to $HOST:/media/cryptofs/apps"
$SCP "$WORK/data.tar.gz" "$SSH_TARGET:/tmp/familyboard-data.tar.gz"
$SSH "$SSH_TARGET" \
  'cd /media/cryptofs/apps && tar -xzf /tmp/familyboard-data.tar.gz && rm -f /tmp/familyboard-data.tar.gz && echo installed'

# A manual tar-copy bypasses appinstalld, which is what normally generates the
# app's LS2 role from appinfo.json. Without a role the webOS Flutter embedder's
# WebOSServiceBridge can't register on the LS2 bus and the app aborts (SIGABRT)
# at view-controller creation — before any Dart runs. So we deploy the role /
# client-perms / manifest into the elevated /var/luna-service2/ ourselves, the
# same way the a2ui reference does, then reload ls-hubd.
echo "[3/5] Deploying LS2 role/perms/manifest to /var/luna-service2"
$SCP "$PERMS_DIR/${APP_ID}.role.json"     "$SSH_TARGET:/var/luna-service2/roles.d/${APP_ID}.app.json"
$SCP "$PERMS_DIR/${APP_ID}.client.json"   "$SSH_TARGET:/var/luna-service2/client-permissions.d/${APP_ID}.app.json"
$SCP "$PERMS_DIR/${APP_ID}.manifest.json" "$SSH_TARGET:/var/luna-service2/manifests.d/${APP_ID}.json"
$SSH "$SSH_TARGET" 'killall -HUP ls-hubd' || true
sleep 1

echo "[4/5] Closing existing instance (if any)"
# Best-effort (nothing to close on a fresh install). closeByAppId is the clean
# path; kill -9 is the backup since SAM can lag. Trailing ':' keeps the remote
# exit status 0 so `set -e` doesn't abort.
$SSH "$SSH_TARGET" "
  luna-send -n 1 -w 3000 luna://com.webos.applicationManager/closeByAppId '{\"id\":\"$APP_ID\"}' >/dev/null 2>&1 || true
  for _ in 1 2 3 4 5; do
    PIDS=\$(pgrep -f 'flutter-client.*$APP_ID' 2>/dev/null) || true
    [ -z \"\$PIDS\" ] && break
    echo \"\$PIDS\" | xargs -r kill -9 2>/dev/null || true
    sleep 1
  done
  :
" || true

echo "[5/5] Launching $APP_ID on $HOST"
# Prefer ares-launch when the target is a registered alias; for a bare IP (not
# in ares) go straight to luna-send over root ssh.
if printf '%s' "$TV_ALIAS" | grep -qvE '^[0-9]+(\.[0-9]+){3}$' && [ -z "${TV_HOST:-}" ]; then
  ares-launch --device "$TV_ALIAS" "$APP_ID" 2>/dev/null || \
    $SSH "$SSH_TARGET" "luna-send -n 1 -f luna://com.webos.service.applicationManager/launch '{\"id\":\"$APP_ID\"}'" || true
else
  $SSH "$SSH_TARGET" "luna-send -n 1 -f luna://com.webos.service.applicationManager/launch '{\"id\":\"$APP_ID\"}'" || true
fi

echo
echo "Done. NEXT: restart the TV's Lisa daemon / appmcp-server so it"
echo "rescans /media/cryptofs/apps and registers familyboard's tools."
