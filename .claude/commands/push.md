# Push to Remote

Push committed changes to the remote repository.

## Steps

1. **Check current branch and remote tracking**
   ```bash
   git branch -vv
   ```

2. **Check if there are commits to push**
   ```bash
   git status
   git log origin/$(git branch --show-current)..HEAD --oneline 2>/dev/null || echo "No upstream branch"
   ```

3. **Push to remote**
   ```bash
   git push
   ```

   If no upstream branch:
   ```bash
   git push -u origin $(git branch --show-current)
   ```

4. **Verify push**
   ```bash
   git status
   ```

## Notes

- Never force push to main/master
- Report the number of commits pushed
- Show the remote URL for reference
