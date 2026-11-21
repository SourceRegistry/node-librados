# [1.1.0](https://github.com/SourceRegistry/node-librados/compare/v1.0.3...v1.1.0) (2026-11-21)


### Bug Fixes

* **build:** include <stdexcept> for std::runtime_error ([#1](https://github.com/SourceRegistry/node-librados/issues/1)) ([0d44095](https://github.com/SourceRegistry/node-librados/commit/0d4409513d258608a3cab739cf07c95db4a5d06f))
* **rados:** refuse io context setting changes while operations are in flight ([#2](https://github.com/SourceRegistry/node-librados/issues/2)) ([dd3f83b](https://github.com/SourceRegistry/node-librados/commit/dd3f83b59c3741a1a7bcc54c97f6da892a6736fd))


### Features

* name the errno of a CephError in codeName ([#5](https://github.com/SourceRegistry/node-librados/issues/5)) ([ff3568d](https://github.com/SourceRegistry/node-librados/commit/ff3568d7c2e499bec6ffee7e0e0eb9128ad1d70f))

## [1.0.3](https://github.com/SourceRegistry/node-librados/compare/v1.0.2...v1.0.3) (2026-09-06)


### Bug Fixes

* **rbd:** trim null padding from rbd_metadata_get result ([1aae63e](https://github.com/SourceRegistry/node-librados/commit/1aae63eeab260fae80b513858a6c4e2db5ed6df8))

## [1.0.2](https://github.com/SourceRegistry/node-librados/compare/v1.0.1...v1.0.2) (2026-09-06)


### Bug Fixes

* **ci:** write microceph config/keyring via tee, not snap -o path ([cb77ee4](https://github.com/SourceRegistry/node-librados/commit/cb77ee4da2034cbebc79b973c1c6260aa62188f4))

## [1.0.1](https://github.com/SourceRegistry/node-librados/compare/v1.0.0...v1.0.1) (2026-09-05)


### Bug Fixes

* **LICENSE:** LICENSE ([98d288d](https://github.com/SourceRegistry/node-librados/commit/98d288d1b6247b8f6a5a36ad31870165043e2937))

# 1.0.0 (2026-09-05)


### Bug Fixes

* fall back to NPM_TOKEN when OIDC trusted publishing unavailable ([2d1ae4f](https://github.com/SourceRegistry/node-librados/commit/2d1ae4fd9fcd926e2c6e2c8df553e0c136b2de10))
* package-lock.json ([13e68fd](https://github.com/SourceRegistry/node-librados/commit/13e68fd423a1100a3a5aed8869ff64e1257a08cd))
* run node-gyp configure before build in build:native script ([ab57abc](https://github.com/SourceRegistry/node-librados/commit/ab57abcbc740ea824f4aaef07826fdce0f8a4c66))


### Features

* add librados and librbd bindings ([b6059e4](https://github.com/SourceRegistry/node-librados/commit/b6059e485d6a7c5375b9f983d3060653e86bb3a2))
* add semantic-release for automated npm/GitHub releases ([0a08a3f](https://github.com/SourceRegistry/node-librados/commit/0a08a3f5d2f5c334788efdab7364746cf300b655))
* switch npm publish to OIDC trusted publishing ([63bbab5](https://github.com/SourceRegistry/node-librados/commit/63bbab5e4e09321d9be2c71d11c20d1e273a294f))
