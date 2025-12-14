$ErrorActionPreference = "Stop"

# remember where we started
$currentBranch = (git rev-parse --abbrev-ref HEAD).Trim()

Write-Host "`nCurrent branch: $currentBranch"
Write-Host "`nMerging develop to master branch..."

git checkout master

git merge develop
if ($LASTEXITCODE -ne 0) {
    Write-Host "`nMerge failed (likely conflicts). Resolve conflicts and re-run. Skipping push."

    # stay on master so conflicts can be resolved here
    exit 1
}

git push origin master

# Switch back only if everything succeeded
if ($currentBranch -ne "master") {
    git checkout $currentBranch
}

Write-Host "`nMerge complete. Returned to: $currentBranch"
