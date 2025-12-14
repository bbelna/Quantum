$ErrorActionPreference = "Stop"

# remember where we started
$currentBranch = (git rev-parse --abbrev-ref HEAD).Trim()

Write-Host "`Current branch is $currentBranch"
Write-Host "`Checking out master..."

git checkout master

# record commit before merge
$before = (git rev-parse HEAD).Trim()

Write-Host "`Merging develop..."

git merge develop

if ($LASTEXITCODE -ne 0) {
    Write-Host "`Merge failed (likely conflicts). Resolve conflicts and re-run. Skipping push."

    # stay on master so conflicts can be resolved here
    exit 1
} else {
    Write-Host "`Merge successful."
}

# record commit after merge
$after = (git rev-parse HEAD).Trim()

# only push if there were new commits merged
if ($before -ne $after) {
    Write-Host "`Pushing master..."

    git push origin master

    Write-Host "`Pushed master (new commits were merged)."
} else {
    Write-Host "`No changes to merge; skipping push."
}

# return to original branch
if ($currentBranch -ne "master") {
    Write-Host "`Returning to $currentBranch"

    git checkout $currentBranch
}
