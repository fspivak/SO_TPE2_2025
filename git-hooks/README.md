# Git Hooks - Instalacion

## Instalacion Rapida

```bash
./git-hooks/install-hooks.sh
```

## Que Se Instala

### Hook pre-commit

**Se ejecuta antes de cada commit:**
1. ✅ Compila todo el proyecto
2. ✅ Verifica errores → Bloquea commit si encuentra
3. ✅ Verifica warnings → Bloquea commit si encuentra
4. ✅ Formatea codigo con clang-format
5. ✅ Hace stage de archivos formateados
6. ✅ Permite que el commit proceda

## Saltar Hook (Una Vez)

```bash
git commit --no-verify -m "WIP: guardado rapido"
```

## Deshabilitar Hook Permanentemente

```bash
chmod -x .git/hooks/pre-commit
```

## Rehabilitar Hook

```bash
chmod +x .git/hooks/pre-commit
```

## Mas Informacion

Ver: `docs/GIT_HOOKS.md`

