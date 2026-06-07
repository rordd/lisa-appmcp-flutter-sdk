import 'dart:convert';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:lisa_appmcp_flutter_sdk/lisa_appmcp_flutter_sdk.dart';

// ──────────────────────────────────────────────
// Memo Model
// ──────────────────────────────────────────────

class Memo {
  final String id;
  final String message;
  final String from;
  final String to;
  final bool pinned;
  final DateTime createdAt;

  Memo({
    required this.id,
    required this.message,
    required this.from,
    required this.to,
    this.pinned = false,
    DateTime? createdAt,
  }) : createdAt = createdAt ?? DateTime.now();

  Map<String, dynamic> toJson() => {
        'id': id,
        'message': message,
        'from': from,
        'to': to,
        'pinned': pinned,
        'createdAt': formatTime(createdAt),
      };

  /// Round-trippable form for on-disk persistence (ISO timestamp, unlike
  /// [toJson] which emits a human-readable time for the LLM).
  Map<String, dynamic> toStore() => {
        'id': id,
        'message': message,
        'from': from,
        'to': to,
        'pinned': pinned,
        'createdAt': createdAt.toIso8601String(),
      };

  factory Memo.fromStore(Map<String, dynamic> j) => Memo(
        id: j['id'] as String? ?? 'memo_${DateTime.now().microsecondsSinceEpoch}',
        message: j['message'] as String? ?? '',
        from: j['from'] as String? ?? '익명',
        to: j['to'] as String? ?? '모두',
        pinned: j['pinned'] as bool? ?? false,
        createdAt:
            DateTime.tryParse(j['createdAt'] as String? ?? '') ?? DateTime.now(),
      );
}

String formatTime(DateTime dt) {
  return '${dt.month}/${dt.day} ${dt.hour.toString().padLeft(2, '0')}:${dt.minute.toString().padLeft(2, '0')}';
}

/// Persists memos to a JSON file so the board survives app restarts.
/// Tries writable+persistent locations in order (webOS app runs as root with
/// /home/root persistent; desktop dev uses $HOME), falling back to temp.
class MemoStore {
  File? _cached;

  File _file() {
    if (_cached != null) return _cached!;
    final candidates = <String>[
      if (Platform.environment['HOME'] != null)
        '${Platform.environment['HOME']}/.family_board',
      '/home/root/.family_board', // webOS persistent root home
      '${Directory.systemTemp.path}/family_board',
    ];
    for (final dirPath in candidates) {
      try {
        Directory(dirPath).createSync(recursive: true);
        final probe = File('$dirPath/.probe');
        probe.writeAsStringSync('ok');
        probe.deleteSync();
        return _cached = File('$dirPath/memos.json');
      } catch (_) {
        continue;
      }
    }
    return _cached = File('${Directory.systemTemp.path}/family_board_memos.json');
  }

  /// Returns persisted memos, or null if none/unreadable (first run).
  List<Memo>? load() {
    try {
      final f = _file();
      if (!f.existsSync()) return null;
      final data = jsonDecode(f.readAsStringSync()) as List;
      return data
          .map((e) => Memo.fromStore(e as Map<String, dynamic>))
          .toList();
    } catch (_) {
      return null;
    }
  }

  Future<void> save(List<Memo> memos) async {
    try {
      await _file()
          .writeAsString(jsonEncode(memos.map((m) => m.toStore()).toList()));
    } catch (_) {
      // Persistence is best-effort; never break the UI on I/O failure.
    }
  }
}

// ──────────────────────────────────────────────
// Family colors & avatars
// ──────────────────────────────────────────────

// Post-it paper color per family member (light, so dark text reads on a TV).
const _familyPaper = <String, Color>{
  '엄마': Color(0xFFF7B7CE), // pink
  '아빠': Color(0xFFAFCBF6), // blue
  '아들': Color(0xFFB7E0B9), // green
  '딸': Color(0xFFFFD79A), // orange
};
const _defaultPaper = Color(0xFFFFE98A); // classic sticky-note yellow

const _familyEmoji = <String, String>{
  '엄마': '👩',
  '아빠': '👨',
  '아들': '👦',
  '딸': '👧',
};

Color _paperFor(String name) => _familyPaper[name] ?? _defaultPaper;
String _emojiFor(String name) => _familyEmoji[name] ?? '👤';

/// Subtle per-note tilt so the board feels like real stuck-on post-its.
double _tiltFor(int i) {
  const angles = [-0.025, 0.018, -0.014, 0.028, -0.03, 0.012];
  return angles[i % angles.length];
}

// ──────────────────────────────────────────────
// App
// ──────────────────────────────────────────────

void main() => runApp(const FamilyBoardApp());

class FamilyBoardApp extends StatelessWidget {
  const FamilyBoardApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: '가족 게시판',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        brightness: Brightness.light,
        colorSchemeSeed: const Color(0xFF6750A4),
        useMaterial3: true,
      ),
      home: const BoardScreen(),
    );
  }
}

class BoardScreen extends StatefulWidget {
  const BoardScreen({super.key});

  @override
  State<BoardScreen> createState() => _BoardScreenState();
}

class _BoardScreenState extends State<BoardScreen> {
  late final LisaMcpClient _client;
  final MemoStore _store = MemoStore();

  final List<Memo> _memos = [
    Memo(
      id: 'init_1',
      message: '저녁 7시에 외식! 준비해~',
      from: '엄마',
      to: '모두',
      pinned: true,
      createdAt: DateTime.now().subtract(const Duration(hours: 2)),
    ),
    Memo(
      id: 'init_2',
      message: '우유 사와주세요',
      from: '아빠',
      to: '엄마',
      createdAt: DateTime.now().subtract(const Duration(hours: 1)),
    ),
    Memo(
      id: 'init_3',
      message: '숙제 다 했어요!',
      from: '아들',
      to: '엄마',
      createdAt: DateTime.now().subtract(const Duration(minutes: 30)),
    ),
  ];

  int _nextId = 1;

  @override
  void initState() {
    super.initState();
    _loadMemos();
    _client = LisaMcpClient(
      LisaMcpConfig(
        appId: 'com.webos.app.familyboard',
        port: 9100,
        autoReconnect: true,
      ),
    );
    _registerTools();
    _client.connect().catchError((_) {});
  }

  /// Restore memos from disk (or persist the seed set on first run), and
  /// advance the id counter past any restored memo_N to avoid collisions.
  void _loadMemos() {
    final saved = _store.load();
    if (saved != null) {
      _memos
        ..clear()
        ..addAll(saved);
    } else {
      _store.save(_memos);
    }
    for (final m in _memos) {
      final match = RegExp(r'^memo_(\d+)$').firstMatch(m.id);
      if (match != null) {
        final n = int.parse(match.group(1)!);
        if (n >= _nextId) _nextId = n + 1;
      }
    }
  }

  void _deleteMemo(String id) {
    setState(() => _memos.removeWhere((m) => m.id == id));
    _store.save(_memos);
  }

  @override
  void dispose() {
    _client.disconnect();
    _client.dispose();
    super.dispose();
  }

  void _registerTools() {
    _client.registerTool(McpToolDef(
      name: 'post_memo',
      description: '가족 게시판에 메모 남기기. 누가 누구에게 남기는지 지정 가능.',
      inputSchema: {
        'type': 'object',
        'properties': {
          'message': {'type': 'string', 'description': '메모 내용'},
          'from': {'type': 'string', 'description': '보내는 사람'},
          'to': {'type': 'string', 'description': '받는 사람 (생략 시 모두)'},
          'pin': {'type': 'boolean', 'description': '중요 메모 고정'},
        },
        'required': ['message', 'from'],
      },
      handler: (args) async {
        final message = args['message'] as String? ?? '';
        final from = args['from'] as String? ?? '익명';
        final to = args['to'] as String? ?? '모두';
        final pin = args['pin'] as bool? ?? false;

        if (message.isEmpty) {
          return McpToolResult.error('메모 내용을 입력해주세요');
        }

        final memo = Memo(
          id: 'memo_${_nextId++}',
          message: message,
          from: from,
          to: to,
          pinned: pin,
        );
        setState(() => _memos.add(memo));
        _store.save(_memos);

        final pinLabel = pin ? ' [고정]' : '';
        return McpToolResult.text(
          '메모 등록 완료$pinLabel\nFrom: ${memo.from} → To: ${memo.to}\n"${memo.message}"',
        );
      },
    ));

    _client.registerTool(McpToolDef(
      name: 'read_memos',
      description:
          '가족 게시판 메모 조회. 특정 사람 대상 또는 전체. 반환되는 각 메모 항목에는 id가 포함되며, 메모를 삭제할 때 이 id를 delete_memo의 memoId로 사용한다.',
      inputSchema: {
        'type': 'object',
        'properties': {
          'for': {'type': 'string', 'description': '특정 사람 대상 메모만 조회'},
        },
      },
      handler: (args) async {
        final forPerson = args['for'] as String?;
        final isAll = forPerson == null ||
            forPerson.isEmpty ||
            const {'all', '모두', '전체'}.contains(forPerson.toLowerCase());
        var filtered = _memos.toList();
        if (!isAll) {
          filtered = _memos
              .where((m) =>
                  m.to == '모두' ||
                  m.to.contains(forPerson!) ||
                  m.from.contains(forPerson))
              .toList();
        }
        if (filtered.isEmpty) {
          return McpToolResult.text('게시판이 비어있습니다');
        }
        filtered.sort((a, b) {
          if (a.pinned && !b.pinned) return -1;
          if (!a.pinned && b.pinned) return 1;
          return b.createdAt.compareTo(a.createdAt);
        });
        return McpToolResult.text(
            jsonEncode(filtered.map((m) => m.toJson()).toList()));
      },
    ));

    _client.registerTool(McpToolDef(
      name: 'delete_memo',
      description:
          '가족 게시판 메모 삭제. memoId가 반드시 필요하다. 사용자가 메모 내용(예: \'잘있어\')으로 삭제 대상을 지목하면, 먼저 read_memos를 호출해 그 내용과 일치하는 메모의 id를 찾은 뒤 그 id를 memoId로 넘겨 호출한다. 이 도구는 항상 사용 가능하므로 \'연결되지 않았다\'고 답하지 말고, 도구가 보이지 않으면 tool_search로 delete_memo를 먼저 검색해 호출할 것.',
      inputSchema: {
        'type': 'object',
        'properties': {
          'memoId': {'type': 'string', 'description': '삭제할 메모 ID'},
        },
        'required': ['memoId'],
      },
      handler: (args) async {
        final id = args['memoId'] as String? ?? '';
        final idx = _memos.indexWhere((m) => m.id == id);
        if (idx == -1) {
          return McpToolResult.error('메모를 찾을 수 없습니다: $id');
        }
        final removed = _memos[idx];
        setState(() => _memos.removeAt(idx));
        _store.save(_memos);
        return McpToolResult.text(
            '메모 삭제 완료: "${removed.message}" (by ${removed.from})');
      },
    ));
  }

  /// Memos of one pinned-state, oldest first so newly added ones go to the end.
  List<Memo> _section(bool pinned) {
    final list = _memos.where((m) => m.pinned == pinned).toList();
    list.sort((a, b) => a.createdAt.compareTo(b.createdAt));
    return list;
  }

  @override
  Widget build(BuildContext context) {
    final pinned = _section(true);
    final others = _section(false);
    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [Color(0xFFEDF1F7), Color(0xFFD8DEE9)],
          ),
        ),
        child: SafeArea(
          child: Padding(
            padding: const EdgeInsets.fromLTRB(56, 36, 56, 36),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                _header(),
                const SizedBox(height: 28),
                Expanded(
                  child: _memos.isEmpty
                      ? const Center(
                          child: Text('아직 메모가 없어요',
                              style: TextStyle(
                                  fontSize: 30, color: Colors.black38)))
                      : SingleChildScrollView(
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              if (pinned.isNotEmpty) ...[
                                _sectionLabel('📌 고정'),
                                const SizedBox(height: 18),
                                _noteWrap(pinned),
                                const SizedBox(height: 30),
                                Container(
                                  height: 2,
                                  color: Colors.black.withValues(alpha: 0.08),
                                ),
                                const SizedBox(height: 30),
                              ],
                              _sectionLabel('메모'),
                              const SizedBox(height: 18),
                              others.isEmpty
                                  ? Padding(
                                      padding:
                                          const EdgeInsets.symmetric(vertical: 8),
                                      child: Text('고정 외 메모가 없어요',
                                          style: TextStyle(
                                              fontSize: 22,
                                              color: Colors.black
                                                  .withValues(alpha: 0.35))),
                                    )
                                  : _noteWrap(others),
                            ],
                          ),
                        ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }

  Widget _sectionLabel(String text) => Text(
        text,
        style: const TextStyle(
            fontSize: 26,
            fontWeight: FontWeight.w700,
            color: Color(0xFF4A5160)),
      );

  Widget _noteWrap(List<Memo> list) => Wrap(
        spacing: 30,
        runSpacing: 30,
        children: [
          for (var i = 0; i < list.length; i++)
            _MemoNote(
              memo: list[i],
              index: i,
              onDelete: () => _deleteMemo(list[i].id),
            ),
        ],
      );

  Widget _header() {
    return Row(
      children: [
        const Text('🏠', style: TextStyle(fontSize: 44)),
        const SizedBox(width: 14),
        const Text('가족 게시판',
            style: TextStyle(
                fontSize: 44,
                fontWeight: FontWeight.w800,
                color: Color(0xFF2B2F38))),
      ],
    );
  }
}

/// A post-it note stuck on the board: colored paper sized to its content,
/// tilted slightly with a drop shadow. Hovering (mouse / Magic Remote pointer)
/// lifts the note and reveals a delete button. Pinned notes are larger/upright.
class _MemoNote extends StatefulWidget {
  final Memo memo;
  final int index;
  final VoidCallback onDelete;
  const _MemoNote({
    required this.memo,
    required this.index,
    required this.onDelete,
  });

  @override
  State<_MemoNote> createState() => _MemoNoteState();
}

class _MemoNoteState extends State<_MemoNote> {
  bool _hover = false;

  @override
  Widget build(BuildContext context) {
    final memo = widget.memo;
    final paper = _paperFor(memo.from);
    final isPinned = memo.pinned;
    return MouseRegion(
      onEnter: (_) => setState(() => _hover = true),
      onExit: (_) => setState(() => _hover = false),
      child: Transform.rotate(
        angle: isPinned ? 0.0 : _tiltFor(widget.index),
        child: Stack(
          children: [
            AnimatedContainer(
              duration: const Duration(milliseconds: 120),
              width: isPinned ? 470 : 340,
              constraints: const BoxConstraints(minHeight: 150),
              padding: const EdgeInsets.fromLTRB(26, 22, 26, 22),
              decoration: BoxDecoration(
                color: paper,
                borderRadius: BorderRadius.circular(6),
                border: Border.all(
                  color: _hover
                      ? Colors.black.withValues(alpha: 0.25)
                      : Colors.transparent,
                  width: 2,
                ),
                boxShadow: [
                  BoxShadow(
                    color: Colors.black.withValues(alpha: _hover ? 0.30 : 0.18),
                    blurRadius: _hover ? 24 : 16,
                    offset: Offset(0, _hover ? 12 : 8),
                  ),
                ],
              ),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      Text(_emojiFor(memo.from),
                          style: const TextStyle(fontSize: 32)),
                      const Spacer(),
                      if (isPinned)
                        const Text('📌', style: TextStyle(fontSize: 26)),
                    ],
                  ),
                  const SizedBox(height: 16),
                  Text(
                    memo.message,
                    style: TextStyle(
                      fontSize: isPinned ? 34 : 28,
                      height: 1.3,
                      fontWeight: FontWeight.w500,
                      color: const Color(0xFF23262B),
                    ),
                  ),
                  const SizedBox(height: 18),
                  Text(
                    formatTime(memo.createdAt),
                    style: TextStyle(
                      fontSize: 16,
                      color: Colors.black.withValues(alpha: 0.45),
                    ),
                  ),
                ],
              ),
            ),
            if (_hover)
              Positioned(
                top: 8,
                right: 8,
                child: _DeleteButton(onTap: widget.onDelete),
              ),
          ],
        ),
      ),
    );
  }
}

/// Round red delete button shown on a hovered note.
class _DeleteButton extends StatelessWidget {
  final VoidCallback onTap;
  const _DeleteButton({required this.onTap});

  @override
  Widget build(BuildContext context) {
    return Material(
      color: Colors.transparent,
      child: InkWell(
        onTap: onTap,
        customBorder: const CircleBorder(),
        child: Container(
          width: 44,
          height: 44,
          alignment: Alignment.center,
          decoration: BoxDecoration(
            color: const Color(0xFFE53935),
            shape: BoxShape.circle,
            boxShadow: [
              BoxShadow(
                color: Colors.black.withValues(alpha: 0.3),
                blurRadius: 8,
                offset: const Offset(0, 3),
              ),
            ],
          ),
          // Plain '×' (system font) — avoids MaterialIcons, which isn't bundled
          // on this app and renders as a stray CJK glyph on webOS.
          child: const Text(
            '×',
            style: TextStyle(
              color: Colors.white,
              fontSize: 32,
              fontWeight: FontWeight.w700,
              height: 1.0,
            ),
          ),
        ),
      ),
    );
  }
}
