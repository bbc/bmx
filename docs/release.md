# Release Process

This describes the steps for making a release.

## Create a Release PR

* Create a branch that will contain changes for the release
* Update the [CHANGELOG.md](../CHANGELOG.md) using the most recent version as a guide
    * Go through the PRs since the last release and add each PR and descriptive text to the `Breaking changes`, `Features`, `Bug fixes` or `Build changes` sections
* Update the versions in the 3 main CMakeLists.txt in the `project` blocks, where `VERSION` has the form `<major version>.<minor version>`
    * [bmx CMakeLists.txt](../CMakeLists.txt), [libMXF CmakeLists.txt](../deps/libMXF/CMakeLists.txt) and [libMXFpp CmakeLists.txt](../deps/libMXFpp/CMakeLists.txt)
* Check that the build & tests succeed and fix any build warnings
* Check the [runner versions](https://docs.github.com/en/actions/using-github-hosted-runners/using-github-hosted-runners/about-github-hosted-runners) (e.g. `windows-2019` and `macos-13`) in the [release workflow](../.github/workflows/release.yml) are still available and update the workflow if not
* Run the `Release` workflow in GitHub Actions using this PR's branch to check it succeeds
* Merge the PR into `main`

## Create a Release Tag

* Checkout and fetch the `main` branch
* Create a tag with form `v<major version>.<minor version>`, e.g. export `BMX_VERSION` (without the `v`), replacing `<major version>.<minor version>` below

```bash
export BMX_VERSION=<major version>.<minor version>
git checkout main
git pull --rebase
git tag -a v${BMX_VERSION} -m "Version ${BMX_VERSION}"`
git push origin v${BMX_VERSION}
```

## Create the Release Packages

* Run the `Release` workflow in GitHub Actions
* Download the Artifacts and extract the individual source and binary zips for the release

## Create a GitHub Release

* Create a new release
* Write the release text using the previous release's text as a starting point
    * Select the `v<major version>.<minor version>` tag
    * Change the CHANGELOG link
    * Update the zip filenames with the new version
    * Update the compiler versions used for the binaries
        * These can be found in the actions output in the `Win64 binary release` and `MacOS Universal binary release` build steps in the 2 jobs of the `Release` workflow in GitHub Actions
* Upload the source and binary zips to the release

## Create a Docker Image for the GitHub Container Registry

* Aquire a Linux machine with Docker installed
* Build the `bmxtools` image and tag using the commands below
    * `BMX_VERSION` should be set as before; note also the `.0` for the patch version.

```bash
DOCKER_BUILDKIT=1 docker build -t bmxtools .
docker tag bmxtools ghcr.io/bbc/bmxtools:${BMX_VERSION}.0
docker tag bmxtools ghcr.io/bbc/bmxtools:latest
```

* On your GitHub personal page, go to "Settings" (top right) -> "Developer settings" (bottom) -> "Personal Access Tokens" / "Tokens (classic)"
    * Select "Generate New Token" and "Generate new token (classic)"
        * Change "Expiration" to 7 days
        * Select "write:packages" (which will select "read:packages" and all of "repo")
    * Select "Generate Token"
    * Copy the token into your clipboard
* Login into GHCR and push the image using the commands below
    * Replace `<username>` with your GitHub username
    * Pass in the token when docker login requests it
    * `BMX_VERSION` should be set as before; note also the `.0` for the patch version

```bash
docker login ghcr.io --username <username>
docker push ghcr.io/bbc/bmxtools:${BMX_VERSION}.0
docker push ghcr.io/bbc/bmxtools:latest
docker logout ghcr.io
```
