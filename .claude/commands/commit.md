# Commit Changes

Create a git commit with an auto-generated commit message.

## Steps

1. **Check git status**
   ```bash
   git status
   ```

2. **Check staged and unstaged changes**
   ```bash
   git diff --cached
   git diff
   ```

3. **Review recent commit history for style reference**
   ```bash
   git log --oneline -5
   ```

4. **Stage all changes** (if user confirms)
   ```bash
   git add -A
   ```

5. **Generate commit message**
   - Analyze the changes
   - Write a concise message (1-2 sentences)
   - Focus on "why" not "what"
   - Follow existing commit style

6. **Create commit**
   ```bash
   git commit -m "$(cat <<'EOF'
   [Generated message here]

   🤖 Generated with [Claude Code](https://claude.com/claude-code)

   Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
   EOF
   )"
   ```

7. **Verify commit**
   ```bash
   git status
   git log -1
   ```

## Notes

- Do NOT push automatically
- Ask before staging untracked files
- Warn about sensitive files (.env, credentials, etc.)
