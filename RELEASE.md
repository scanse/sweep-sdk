# Publishing a new version

- [ ] Sync. `CHANGELOG.md` with latest changes and fixes
- [ ] Sync. version number in `libsweep/CMakeLists.txt`, `sweeppy/setup.py`, `sweepjs/package.json`
- [ ] Make sure Travis and AppVeyor are green and show no warnings
- [ ] Continuous Integration only tests dummy library: test with real device
- [ ] Tag a commit `git tag -a vx.y.z gitsha` (we use [semantic versioning](http://semver.org/))
- [ ] Push tag to Github `git push origin vx.y.z`
- [ ] Head over to https://github.com/scanse/sweep-sdk/releases and make a release
