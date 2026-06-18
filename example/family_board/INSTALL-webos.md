# Family Board — webOS TV 설치 가이드

`com.webos.app.familyboard` (Flutter) 앱을 webOS TV에 설치하고, Lisa가
`appmcp-server`를 통해 앱의 툴(`post_memo`/`read_memos`/`delete_memo`)을 호출하도록
띄우는 절차다.

```
Lisa (TV daemon) ──stdio MCP──> appmcp-server ──luna-send launch──> Family Board app
                                      ▲  ws://localhost:9100/mcp/com.webos.app.familyboard
                                      └──────────────── 앱이 접속해서 tools/call 처리
```

## 사전 요구사항

- 빌드 머신: `flutter-webos` SDK (`~/flutter-webos/bin/flutter-webos`, Flutter 3.27.3), `ares-cli`.
- 대상 TV:
  - dev 모드 + **root ssh 접근(포트 22)**. 수동 설치가 `/media/cryptofs/apps` 쓰기와 `luna-send` 실행에 root를 쓴다. (ares dev 계정 `prisoner@...:9922`와 별개 경로)
  - `~/.local`이 아니라 `/home/root/lisa/appmcp-server`가 `--platform webos`로 실행 중이어야 함 (보통 TV의 Lisa 데몬이 자식으로 띄움).
  - `ares-setup-device`에 대상이 등록돼 있어야 함 — 설치 스크립트가 거기서 **호스트 IP만** 읽는다. (예: `tv_a2ui` → 192.168.0.6)

> appId는 `com.webos.app.familyboard` 고정 — `appMCP.json`·`appinfo.json`·디렉터리명이 모두 일치해야 하며 하이픈 금지. `com.webos.*` prefix라 `ares-install`이 dev 모드에서 거부하므로 **수동 설치**를 쓴다.

## 1. IPK 빌드

```bash
cd example/family_board
unset LD_LIBRARY_PATH                 # flutter-webos가 LD_LIBRARY_PATH 설정 시 거부함
~/flutter-webos/bin/flutter-webos build webos --release --no-tree-shake-icons --ipk
# 산출물: build/webos/arm/release/ipk/com.webos.app.familyboard.ipk
```

`webos/meta/appMCP.json`이 IPK 앱 루트로 동봉되므로(= `appmcp-server`가 읽는 매니페스트),
별도 복사가 필요 없다. 단 `webos/meta/appMCP.json`은 루트 `appMCP.json`과 손으로 동기화해야 한다.

## 2. 설치 (스크립트)

```bash
./scripts/install-tv.sh tv_a2ui       # ares 별칭 (인자 생략 시 기본 tv_a2ui)
./scripts/install-tv.sh 192.168.0.37   # IP 직접 지정 — ares 미등록 TV도 가능
# ssh 계정/포트 override가 필요하면:
SSH_USER=root SSH_PORT=22 ./scripts/install-tv.sh <ip-or-alias>
```

> 인자가 IP면 ares 조회를 건너뛰고 `root@<ip>:22`로 바로 붙는다(별칭이면 ares에서 IP만 해석). `TV_HOST=<ip>`로도 지정 가능.

스크립트가 하는 일 (root@<ip>:22 기준):

1. IPK를 로컬에서 `ar`로 풀어 `data.tar.gz` 추출.
2. `data.tar.gz`를 TV로 scp → `/media/cryptofs/apps`에서 untar → 앱이 `/media/cryptofs/apps/usr/palm/applications/com.webos.app.familyboard/`에 설치됨 (이 경로를 webOS `appmcp-server`가 스캔).
3. **LS2 perms 배포** — `scripts/luna-perms/`의 role/client/manifest를 `/var/luna-service2/{roles.d,client-permissions.d,manifests.d}`에 scp → `killall -HUP ls-hubd`. **필수**: 수동 tar 복사는 `appinstalld`를 우회해 앱의 LS2 role이 자동 생성되지 않는다. role이 없으면 임베더의 `WebOSServiceBridge`가 시작 시 LS2 등록에 실패해 **SIGABRT(앱 즉시 크래시)** — Dart가 돌기도 전에 죽는다.
4. 기존 인스턴스 종료(`closeByAppId` + `kill -9`).
5. 앱 launch (`ares-launch`; 실패 시 `luna-send applicationManager/launch`로 fallback).

### 스크립트 없이 수동으로

```bash
IPK=build/webos/arm/release/ipk/com.webos.app.familyboard.ipk
W=$(mktemp -d); cp "$IPK" "$W/a.ipk"; (cd "$W" && ar x a.ipk)
scp -P 22 "$W/data.tar.gz" root@<ip>:/tmp/fb.tar.gz
ssh -p 22 root@<ip> 'cd /media/cryptofs/apps && tar -xzf /tmp/fb.tar.gz && rm -f /tmp/fb.tar.gz'
# LS2 perms
scp -P 22 scripts/luna-perms/com.webos.app.familyboard.role.json     root@<ip>:/var/luna-service2/roles.d/com.webos.app.familyboard.app.json
scp -P 22 scripts/luna-perms/com.webos.app.familyboard.client.json   root@<ip>:/var/luna-service2/client-permissions.d/com.webos.app.familyboard.app.json
scp -P 22 scripts/luna-perms/com.webos.app.familyboard.manifest.json root@<ip>:/var/luna-service2/manifests.d/com.webos.app.familyboard.json
ssh -p 22 root@<ip> 'killall -HUP ls-hubd'
```

## 3. 활성화 (툴 등록)

`appmcp-server`는 **시작 시 apps-dir를 스캔**하므로, 설치 후 TV의 Lisa 데몬을 재기동해야
새 앱 매니페스트를 읽어 툴 3개를 등록한다. (데몬이 띄울 때 `appmcp-server`도 같이 재시작됨)

TV에서 (idle일 때):

```bash
ssh root@<ip> 'OLD=$(pgrep -f "zeroclaw daemon"); kill $OLD; sleep 3; \
  cd /home/root/lisa; \
  setsid sh -c "set -a; . /home/root/.zeroclaw/.env; set +a; \
    exec ./zeroclaw daemon >> /tmp/zeroclaw-root.log 2>&1 < /dev/null" & \
  sleep 3; pgrep -af "zeroclaw daemon"'
```

성공 시 `/tmp/zeroclaw-root.log`에 다음이 찍힌다:

```
loaded app manifest app_id=com.webos.app.familyboard name=Family Board tools=3
MCP server `appmcp` connected — 3 tool(s) available
```

## 4. 실행 & 검증

앱 실행 (둘 중 하나):

- TV 리모컨: 홈 → 앱 → Family Board
- Lisa: "가족 게시판 보여줘" → `appmcp-server`가 tool 호출 시 `luna-send`로 lazy-launch

검증 (TV에서, read-only):

```bash
# 1) 앱 프로세스 (이름이 fapp.familyboar 로 잘려 'familyboard' grep엔 안 잡힘)
pgrep -af 'fapp\.fam'
# 2) 크래시 없는지 (설치 시각 이후 신규 리포트 없어야 함)
find /var/log/reports/librdx -name '*familyboar*' -newermt '<설치시각>'
# 3) 앱이 appmcp-server에 접속했는지
grep -a 'app connected.*com.webos.app.familyboard' /tmp/zeroclaw-root.log | tail -1
```

화면 캡처 (UX 확인용):

```bash
ssh root@<ip> "luna-send -t 1 -f luna://com.webos.service.capture/executeOneShot \
  '{\"path\":\"/tmp/cap.jpg\",\"method\":\"SCREEN\",\"width\":1280,\"height\":720,\"format\":\"JPEG\"}'"
scp root@<ip>:/tmp/cap.jpg ./cap.jpg
```

## 데이터 영속화

메모는 `<HOME>/.family_board/memos.json`에 저장된다. webOS에서 앱은 root로 돌고
`HOME=/home/root`이므로 실제 경로는 **`/home/root/.family_board/memos.json`** (재부팅에도 유지).
앱 첫 실행 시 시드 메모로 생성되고, 추가/삭제(음성·MCP·UI)마다 갱신된다. 시드로 초기화하려면 이 파일을 지운다.

## 트러블슈팅

| 증상 | 원인 / 조치 |
|---|---|
| 앱이 뜨자마자 SIGABRT (RDX에 `WebOSServiceBridge` 크래시) | LS2 role 미배포. 위 2-3단계(luna-perms + `ls-hubd` HUP) 수행 후 재실행 |
| Lisa가 familyboard 툴을 못 봄 / `tools=0` | 설치가 데몬 기동 *이후*였음. 3단계(데몬 재기동)로 재스캔 |
| `ares-install` "Cannot install privileged app" | `com.webos.*` prefix는 dev 모드 거부. 수동 설치(이 문서) 사용 |
| `ares-*` 가 `Cannot parse privateKey` | 등록된 dev ssh 키 손상. 수동 설치는 ares 키와 무관한 `root@<ip>:22`를 쓰므로 영향 없음 (필요 시 키 재발급) |
| 앱 launch가 빈 응답 | ad-hoc root ssh 셸에는 `applicationManager/launch` 권한 컨텍스트가 없을 수 있음. TV 리모컨/Lisa로 실행하거나, `appmcp-server`의 lazy-launch로 트리거 |
| description 등 매니페스트 변경이 TV에 반영 안 됨 | 설치된 `appMCP.json`(`/media/cryptofs/apps/.../com.webos.app.familyboard/`)을 scp로 교체 후 **데몬 재기동**(3단계)이면 됨 — description-only 변경은 IPK 재빌드 불필요. (`appmcp-server`는 파일 watcher가 없고, 이미 등록된 appId는 재접속해도 rescan 안 함 → 재기동 필수.) Dart 코드까지 바뀌었으면 1-2단계부터 재빌드·재설치 |
