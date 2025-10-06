#!/bin/bash

# Script para instalar git hooks en el repositorio
# Uso: ./git-hooks/install-hooks.sh

echo "=========================================="
echo "Installing Git Hooks for TPE2 SO"
echo "=========================================="

REPO_ROOT=$(git rev-parse --show-toplevel 2>/dev/null)

if [ -z "$REPO_ROOT" ]; then
    echo "❌ Error: Not in a git repository"
    exit 1
fi

# Copiar pre-commit hook
cp "$REPO_ROOT/git-hooks/pre-commit" "$REPO_ROOT/.git/hooks/pre-commit"
chmod +x "$REPO_ROOT/.git/hooks/pre-commit"

echo ""
echo "✅ Pre-commit hook installed successfully"
echo ""
echo "📋 What it does:"
echo "  1. Compiles project before each commit"
echo "  2. Blocks commit if there are errors or warnings"
echo "  3. Auto-formats code with clang-format"
echo ""
echo "🔧 To skip temporarily: git commit --no-verify"
echo ""
echo "=========================================="

