/*
 * pmtrace_bundle_provider.c
 *
 * Tracepoint provider file for LTTng UST tracing in palmsystem-injection
 *
 * For more information on see:
 *    http://lttng.org/files/doc/man-pages/man3/lttng-ust.3.html
 *
 * The application level API to these tracepoints is in WebAppMgrBundleTracer.h
 *
 * Copyright (c) 2015 LG Electronics, Inc.
 */

/*
 * These #defines alter the behavior of pmtrace_bundle_provider.h to define the tracing
 * primitives rather than just declaring them.
 */
#define TRACEPOINT_CREATE_PROBES
#define TRACEPOINT_DEFINE
/*
 * The header containing our TRACEPOINT_EVENTs.
 */
#include "pmtrace_bundle_provider.h"
