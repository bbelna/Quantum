Write-Host "`nMerging develop to master branch..."

git checkout master
if ($LASTEXITCODE -ne 0) { throw "git checkout master failed." }

git merge develop
if ($LASTEXITCODE -ne 0) {
    Write-Host "`nMerge failed (likely conflicts). Skipping push."
    exit 1
}

git push origin master
if ($LASTEXITCODE -ne 0) { throw "git push failed." }

Write-Host "`nMerge complete."
