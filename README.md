# chromium68
This repository contains the v68 edition of the open source Chromium project used by webOS.

Find more information about open source Chromium in Google's [The Chromium Project](http://www.chromium.org/developers/design-documents/) page.

## How to Build
This repository will be built within OE(Open Embedded) build system of webOS platform.

Note: Currently, chromium68 is not the official webruntime for webOS OSE as its still "Work In Progress".
To enable chromium68 as webruntime when building webOS OSE, you need to follow below steps:
- checkout build-webosose
- create webos-local.conf file in build-webosose with following contents before building build-webosose:
PREFERRED_PROVIDER_virtual/webruntime = "webruntime"
- follow instructions on build-webosose README on how to build image

## Copyright and License Information
See the file src/LICENSE
