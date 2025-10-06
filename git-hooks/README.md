# Git Hooks - Installation

## Quick Install

```bash
./git-hooks/install-hooks.sh
```

## What Gets Installed

### pre-commit Hook

**Runs before each commit:**
1. ✅ Compiles entire project
2. ✅ Checks for errors → Blocks commit if found
3. ✅ Checks for warnings → Blocks commit if found
4. ✅ Formats code with clang-format
5. ✅ Stages formatted files
6. ✅ Allows commit to proceed

## Skip Hook (One Time)

```bash
git commit --no-verify -m "WIP: quick save"
```

## Disable Hook Permanently

```bash
chmod -x .git/hooks/pre-commit
```

## Re-enable Hook

```bash
chmod +x .git/hooks/pre-commit
```

## More Info

See: `docs/GIT_HOOKS.md`

