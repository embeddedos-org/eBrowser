// SPDX-License-Identifier: MIT
//
// Omnibox helpers — Chrome-style "smart" address bar input handling.
// Given raw user input from the URL bar, produce a navigable URL:
//   * already-schemed URLs (http://, https://, file://, chrome://) pass through
//   * hostname-shaped input (foo.com, foo.com/bar, 127.0.0.1, localhost) gets
//     "https://" prepended
//   * everything else is treated as a search query against the configured
//     default search engine (Google by default)
//
// This file is part of Phase A of the chrome-like-browser-features plan.

#ifndef eBrowser_OMNIBOX_H
#define eBrowser_OMNIBOX_H

#include <stdbool.h>
#include <stddef.h>

#define EB_OMNIBOX_DEFAULT_SEARCH "https://www.google.com/search?q="

/**
 * Normalize raw omnibox input into a navigable URL.
 *
 * @param user_input    NUL-terminated text the user typed.
 * @param out           Buffer for the normalized URL.
 * @param outsz         Size of @p out (including NUL).
 * @param search_prefix Search-engine URL prefix that takes a URL-encoded
 *                      query appended (e.g. "https://www.google.com/search?q=").
 *                      If NULL, EB_OMNIBOX_DEFAULT_SEARCH is used.
 * @return true on success, false if @p user_input is empty or buffers are NULL.
 */
bool eb_omnibox_normalize(const char *user_input,
                          char *out,
                          size_t outsz,
                          const char *search_prefix);

/* Returns true if @p s already has a recognised scheme prefix
 * (http, https, file, chrome, about, ftp). */
bool eb_omnibox_has_scheme(const char *s);

/* Returns true if @p s looks like a bare hostname or host[/path] — i.e. the
 * kind of input that should auto-prepend "https://" rather than become a
 * search query. */
bool eb_omnibox_looks_like_url(const char *s);

#endif /* eBrowser_OMNIBOX_H */
