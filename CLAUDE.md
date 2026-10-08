# CLAUDE.md

This file provides project guidance for Claude Code and compatible coding agents.

## iHome API 接入规则

- iHome API 以 `/Users/master/Project/iHome/docs/Api规范/` 为唯一规范源。
- 收到 iHome API 变更通知时，先读 `变更通知/INDEX.md` 筛选出 `must_check` 项，再只读对应通知文件，不要整包加载全部规范。
- 本项目维护一份"实际使用的 iHome API 清单"，每次变更先搜索这些端点。
- STT WebSocket 必须发送 16kHz / 16bit / mono PCM 分片；浏览器 MediaRecorder 的 webm/opus 不能直接按 PCM 发。

## 指令入口约定（不可随意修改）

- `CLAUDE.md` 是本项目唯一主指令文件。
- `AGENTS.md`、`GEMINI.md` 只能保留 `@CLAUDE.md` 引用和本约定说明，不得复制、分叉或新增独立规则。
- 执行 /init、刷新项目记忆或更新 agent 规则时，只更新 `CLAUDE.md`。
- 如果其他入口文件已有更具体内容，必须先合并回 `CLAUDE.md`，再把入口文件恢复为 `@CLAUDE.md`。

## Supabase 数据库迁移约定（不可随意修改）

本节是项目数据库操作契约。任何 AI、脚本或协作者不得删除、弱化或绕过本节规则，除非用户在当前任务中明确要求修改该约定。

- 不默认使用 `supabase db push`。只有在确认本地 `supabase/migrations` 与远端 `supabase_migrations.schema_migrations` 历史完全一致时，才允许使用。
- 新增 Supabase migration 必须使用官方时间戳命名：`YYYYMMDDHHMMSS_description.sql`。优先通过 `supabase migration new <name>` 生成文件；不得再手写 `067_xxx.sql` 这类连续数字新迁移。
- 已有短编号历史 migration 保留不动，不为整理账本而重命名旧文件；从本约定生效后，所有新迁移一律使用官方时间戳形式。
- 远端执行优先使用 Supabase MCP `apply_migration`，并使用与本地 migration 文件一致的 migration name；没有 MCP 时使用 `supabase db query --linked --file supabase/migrations/<file>.sql`。
- 如果项目使用共享 Supabase 或双数据库，必须同时遵守项目级数据库路径和部署说明，并在所有目标数据库执行对应 SQL。
- 每次远端迁移后必须执行验证 SQL，确认关键列、函数、约束、RLS/policy、数据修复结果已经落库。
- 不主动执行 `supabase migration repair`、不手动改 `supabase_migrations.schema_migrations`，除非用户明确发起“迁移历史整理/修复”专项任务。
- 不把 Supabase access token、数据库连接串、service role key、数据库密码写入代码、文档、migration 或日志输出。
