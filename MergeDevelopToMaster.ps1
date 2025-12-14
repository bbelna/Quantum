$ErrorActionPreference = "Stop"

# remember where we started
$currentBranch = (git rev-parse --abbrev-ref HEAD).Trim()

Write-Host "`Current branch: $currentBranch"
Write-Host "`nMerging develop to master branch..."

git checkout master

# record commit before merge
$before = (git rev-parse HEAD).Trim()

git merge develop

if ($LASTEXITCODE -ne 0) {
    Write-Host "`nMerge failed (likely conflicts). Resolve conflicts and re-run. Skipping push."

    # stay on master so conflicts can be resolved here
    exit 1
}

# record commit after merge
$after = (git rev-parse HEAD).Trim()

# only push if there were new commits merged
if ($before -ne $after) {
    git push origin master
    Write-Host "`nPushed master (new commits were merged)."
} else {
    Write-Host "`nNo changes to merge; skipping push."
}

# return to original branch
if ($currentBranch -ne "master") {
    git checkout $currentBranch
}

Write-Host "`Merge complete. Returned to: $currentBranch"
