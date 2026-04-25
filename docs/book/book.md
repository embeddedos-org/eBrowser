---
title: "eBrowser [@garsiel2011] — Official Reference Guide"
author: "Srikanth Patchava & EmbeddedOS Contributors"
date: "April 2026"
version: "v1.0.0"
bibliography: references.bib
csl: ieee.csl
titlepage: true
titlepage-background: "cover.png"
---

# eBrowser — Lightweight Embedded Web Browser

## Product Reference

**Authors:** Srikanth Patchava & EmbeddedOS Contributors

**Edition:** First Edition — April 2026

**License:** MIT License

**Publisher:** EmbeddedOS Project

---

> *"The art of engineering is not in building something complex,*
> *but in making the complex appear simple."*

---

## Table of Contents

1. [Preface](#preface)
2. [Chapter 1: Introduction to eBrowser](#chapter-1-introduction-to-ebrowser)
3. [Chapter 2: Architecture Overview](#chapter-2-architecture-overview)
4. [Chapter 3: The Core Rendering Engine](#chapter-3-the-core-rendering-engine)
5. [Chapter 4: Platform Abstraction Layer](#chapter-4-platform-abstraction-layer)
6. [Chapter 5: The Network Stack](#chapter-5-the-network-stack)
7. [Chapter 6: Input Handling](#chapter-6-input-handling)
8. [Chapter 7: Building and Configuration](#chapter-7-building-and-configuration)
9. [Chapter 8: The Embedding API](#chapter-8-the-embedding-api)
10. [Chapter 9: JavaScript Engine](#chapter-9-javascript-engine)
11. [Chapter 10: Display and Rendering Backend](#chapter-10-display-and-rendering-backend)
12. [Chapter 11: Testing and Quality Assurance](#chapter-11-testing-and-quality-assurance)
13. [Chapter 12: Performance Optimization](#chapter-12-performance-optimization)
14. [Chapter 13: Deployment on Embedded Targets](#chapter-13-deployment-on-embedded-targets)
15. [Appendix A: CLI Reference](#appendix-a-cli-reference)
16. [Appendix B: Configuration Reference](#appendix-b-configuration-reference)
17. [Appendix C: Troubleshooting Guide](#appendix-c-troubleshooting-guide)
18. [Glossary of Terms](#glossary-of-terms)

---

# Preface

## Why This Book Exists

The embedded systems landscape has undergone a dramatic transformation. Devices that
once displayed simple text on monochrome LCDs now drive full-color touchscreens,
render dynamic user interfaces, and connect to cloud services over the Internet.
Yet the fundamental constraints of embedded development — limited memory, restricted
processing power, and tight binary size budgets — remain as real as ever.

eBrowser was born from a simple observation: existing web browser engines are
designed for desktop and mobile platforms with gigabytes of RAM and multi-core
processors. When an embedded engineer needs to display web content on a device
with 512 KB of RAM and a 100 MHz microcontroller, options are practically nonexistent.
The major browser engines — Chromium, WebKit, Gecko — each require hundreds of
megabytes of memory and ship as multi-gigabyte codebases. Even "lightweight"
alternatives like NetSurf or Dillo assume desktop-class resources.

eBrowser fills this gap. Built from the ground up for resource-constrained
environments, it implements a practical subset of web standards sufficient for
rendering structured documents, styled layouts, interactive forms, and basic
scripted interfaces — all within memory budgets measured in kilobytes rather
than gigabytes.

This book serves as the definitive technical reference for eBrowser. Whether you are
integrating eBrowser into an IoT gateway, a smart home panel, an industrial HMI,
a medical device, or a wearable, this reference will guide you through every aspect
of the engine: from high-level architecture to low-level memory optimization, from
build system configuration to bare-metal deployment.

## Who This Book Is For

This book is written for:

- **Embedded Systems Engineers** who need to render web content on constrained hardware
- **IoT Platform Developers** building connected devices with browser-based UIs
- **RTOS Developers** integrating web capabilities into real-time operating systems
- **Technical Leads** evaluating lightweight browser engines for embedded products
- **Open-Source Contributors** seeking to understand the eBrowser codebase in depth
- **Students and Researchers** interested in browser engine internals and embedded systems

We assume familiarity with C/C++ programming, embedded systems concepts, and basic
web technologies (HTML, CSS [@css_layout], JavaScript). No prior experience with browser engine
internals is required — this book will take you from zero to expert.

## Conventions Used in This Book

Throughout this reference, we use the following conventions:

| Convention          | Meaning                                       |
|---------------------|-----------------------------------------------|
| `monospace`         | Code, commands, file paths, and API names     |
| **Bold**            | Important terms and concepts                  |
| *Italic*            | Emphasis, new terms on first use              |
| `[OPTIONAL]`        | Features that can be enabled or disabled      |
| `>>>`               | Shell prompts for command-line examples        |

## Acknowledgments

eBrowser is made possible by the EmbeddedOS community — engineers and contributors
who believe that the web should be accessible on every device, no matter how small.
Special thanks to all contributors who have submitted patches, reported bugs, and
helped shape the architecture described in this book. We also acknowledge the
developers of the open-source libraries that eBrowser optionally integrates:
SDL2, LVGL, and mbedTLS [@rfc8446].

---

# Chapter 1: Introduction to eBrowser

## 1.1 What Is eBrowser?

eBrowser is a minimal, embeddable web browser engine designed specifically for
resource-constrained embedded systems and Internet of Things (IoT) devices. Unlike
full-featured browser engines such as Chromium, WebKit, or Gecko — which require
hundreds of megabytes of RAM and powerful multi-core CPUs — eBrowser is engineered
to provide essential web-rendering capabilities while maintaining a binary footprint
measured in kilobytes and memory usage suitable for devices with as little as 256 KB
of RAM.

At its core, eBrowser implements a subset of the HTML5 [@w3c_html5] and CSS specifications
sufficient for rendering structured documents, styled layouts, and interactive
forms. An optional, lightweight JavaScript engine provides basic scripting support
for devices that require interactivity beyond static page rendering.

eBrowser is part of the broader EmbeddedOS ecosystem, which provides a suite of
tools, libraries, and operating system components for embedded development. While
eBrowser can be used standalone on any platform with a C compiler, it integrates
particularly well with EmbeddedOS for native embedded deployments.

## 1.2 Design Philosophy

eBrowser is guided by three fundamental design principles that inform every
engineering decision:

### Principle 1: Minimal Footprint

Every line of code in eBrowser earns its place. The engine uses no dynamic memory
allocation during steady-state operation beyond what is required for the currently
rendered document. Binary size is kept small through aggressive dead-code elimination,
optional feature toggles, and avoidance of third-party library dependencies where
possible.

| Metric              | eBrowser        | Full Browser Engine  |
|---------------------|-----------------|----------------------|
| Minimum binary size | ~48 KB          | ~50+ MB              |
| Minimum RAM usage   | ~256 KB         | ~200+ MB             |
| Startup time        | < 50 ms         | 1–5 seconds          |
| Lines of code       | ~25,000         | 10,000,000+          |

### Principle 2: Low Memory Usage

eBrowser employs several strategies to minimize memory consumption:

- **Pool-based allocation** for DOM [@dom_spec] nodes and style objects avoids heap fragmentation
- **Streaming HTML parsing** processes documents incrementally rather than loading
  the entire document into memory before parsing begins
- **Shared string interning** deduplicates attribute names and CSS property values
  that appear repeatedly across elements
- **Compact layout structures** use fixed-point arithmetic instead of floating-point,
  reducing per-object memory by 30–40% on 32-bit platforms
- **Configurable cache sizing** allows runtime adjustment of memory usage to fit
  the target device's available RAM

### Principle 3: Portability

The Platform Abstraction Layer (PAL) isolates all OS-specific and hardware-specific
code behind a clean C interface. Porting eBrowser to a new platform requires
implementing approximately 15 functions covering memory allocation, file I/O,
timing, and display output. This design has enabled eBrowser to run on Linux,
FreeRTOS, bare-metal ARM, EmbeddedOS, Zephyr, and NuttX with minimal effort.

## 1.3 Target Devices and Use Cases

eBrowser is designed for a broad spectrum of embedded devices, from powerful
application processors to modest microcontrollers:

| Device Category     | Example Devices               | Typical Specs              |
|---------------------|-------------------------------|----------------------------|
| IoT Gateways        | Smart home hubs, routers      | ARM Cortex-A, 64+ MB RAM  |
| Industrial HMI      | Factory panels, PLCs          | ARM Cortex-M7, 1+ MB RAM  |
| Smart Displays      | Thermostats, control panels   | ESP32-S3, 512 KB RAM      |
| Wearables           | Smartwatches, fitness bands   | ARM Cortex-M4, 256 KB RAM |
| Automotive          | Instrument clusters, IVI      | ARM Cortex-A53, 256+ MB   |
| Medical Devices     | Patient monitors, infusion    | ARM Cortex-R, 2+ MB RAM   |

### Use Case Examples

**Smart Home Panel:** A wall-mounted touch panel running a custom RTOS uses
eBrowser to render a locally hosted dashboard showing temperature, lighting
controls, and security camera status. The panel has a 480x320 display, 2 MB
of RAM, and a 200 MHz ARM Cortex-M7 processor. eBrowser renders the dashboard
HTML served by a local gateway, processes touch input for interactive controls,
and refreshes data every 5 seconds via AJAX-like polling.

**Industrial Gateway:** A factory floor gateway with an embedded Linux system
uses eBrowser to display a configuration web interface. Technicians access
device settings through a 7-inch touchscreen without requiring a full desktop
browser stack. The eBrowser instance renders forms for network configuration,
firmware update status, and diagnostic logs.

**IoT Sensor Hub:** A battery-powered sensor hub uses eBrowser in headless
mode to parse JSON-over-HTTP responses from a cloud API, extracting
configuration updates without rendering any visual output. The HTML parser
handles the response body, while the network stack manages authenticated
HTTPS connections to the cloud service.

**Wearable Device:** A smartwatch with a 240x240 circular display uses
eBrowser to render notification content received as HTML snippets from a
paired smartphone. The minimal footprint allows eBrowser to coexist with
the device's fitness tracking firmware within 256 KB of RAM.

## 1.4 Feature Summary

The following matrix shows all eBrowser features and their availability:

```
+---------------------------------------------+
|          eBrowser Feature Matrix             |
+---------------------------------------------+
| Feature              | Status     | Toggle  |
|----------------------|------------|---------|
| HTML5 subset parsing | Core       | Always  |
| CSS subset rendering | Core       | Always  |
| DOM tree management  | Core       | Always  |
| Block layout         | Core       | Always  |
| Inline layout        | Core       | Always  |
| Form elements        | Core       | Always  |
| Table layout         | Optional   | Flag    |
| Flexbox layout       | Optional   | Flag    |
| JavaScript engine    | Optional   | Flag    |
| HTTPS / TLS          | Optional   | Flag    |
| Image decoding       | Optional   | Flag    |
| Touch input          | Optional   | Flag    |
| Keyboard input       | Optional   | Flag    |
| SDL2 display port    | Optional   | Flag    |
| LVGL display port    | Optional   | Flag    |
| Framebuffer output   | Optional   | Flag    |
| Response caching     | Optional   | Flag    |
| Profiling support    | Optional   | Flag    |
+---------------------------------------------+
```

## 1.5 What eBrowser Is Not

eBrowser deliberately omits capabilities that would compromise its embedded focus:

- **Not a full browser:** eBrowser does not aim for complete HTML5/CSS3 compliance.
  It implements a practical subset sufficient for embedded use cases.
- **Not a JavaScript runtime:** The optional JS engine supports basic DOM
  manipulation and event handling, not full ECMAScript compliance.
- **Not a media player:** Audio and video playback are not supported.
- **Not a PDF renderer:** Document viewing is limited to HTML content.
- **Not a security sandbox:** eBrowser trusts the content it renders. It is
  designed for controlled embedded environments, not general web browsing.
- **Not multi-tabbed:** eBrowser renders a single document at a time.

---

# Chapter 2: Architecture Overview

## 2.1 High-Level Architecture

eBrowser employs a layered architecture that separates concerns cleanly and allows
each layer to be developed, tested, and replaced independently. The following
diagram illustrates the major components and their relationships:

```
+-------------------------------------------------------------+
|                    Application Layer                         |
|               (Embedding API / CLI Interface)                |
+-------------------------------------------------------------+
|                                                              |
|  +-----------+  +-----------+  +-----------+  +-----------+ |
|  |   HTML    |  |   CSS     |  |  Layout   |  |   DOM     | |
|  |  Parser   |  |  Engine   |  |  Engine   |  |   Tree    | |
|  +-----+-----+  +-----+-----+  +-----+-----+  +-----+---+ |
|        |              |              |              |        |
|        +--------------+--------------+--------------+        |
|                    Core Engine Layer                          |
|                                                              |
+----------+--------------+----------------+-------------------+
|          |              |                |                    |
|  +-------+---+  +------+------+  +------+------+            |
|  | Rendering |  |  Network    |  |   Input     |            |
|  | Backend   |  |  Stack      |  |   Layer     |            |
|  +-------+---+  +------+------+  +------+------+            |
|          |              |                |                    |
+----------+--------------+----------------+-------------------+
|              Platform Abstraction Layer (PAL)                 |
|         (Memory, File I/O, Timing, Display, Network)         |
+-------------------------------------------------------------+
|                  Hardware / OS Layer                          |
|        (Linux, RTOS, Bare-Metal, Custom OS, SDL2)            |
+-------------------------------------------------------------+
```

## 2.2 Layer Descriptions

### Application Layer

The topmost layer provides two interfaces for interacting with eBrowser:

1. **Embedding API** — A C API (`eBrowser.h`) that allows host applications to
   create browser instances, navigate to URLs, and process events programmatically.
   This is the primary integration point for embedded applications.
2. **CLI Interface** — A command-line executable that wraps the embedding API
   for standalone usage during development and testing. It supports all runtime
   configuration options via command-line flags.

### Core Engine Layer

The core engine is the heart of eBrowser. It consists of four tightly integrated
subsystems that work together to transform HTML and CSS into a renderable layout:

| Subsystem     | Responsibility                                              |
|---------------|-------------------------------------------------------------|
| HTML Parser   | Tokenizes and parses HTML documents into a DOM tree         |
| CSS Engine    | Parses stylesheets and resolves computed styles per element  |
| Layout Engine | Computes the geometric position and size of every element   |
| DOM Tree      | In-memory representation of the document structure           |

### Rendering Backend

The rendering backend translates the layout tree into pixels. It provides a
display-agnostic interface that can target multiple output devices:

- **Framebuffer** — Direct pixel writes to a memory-mapped framebuffer
- **SDL2** — Desktop rendering for development and testing
- **LVGL** — Integration with the LVGL embedded graphics library (v9.2)
- **Custom** — User-provided rendering callback for specialized displays

### Network Stack

The network stack handles all HTTP and HTTPS communication:

- **HTTP/1.1 client** with keep-alive connection support
- **Optional TLS** via mbedTLS or platform-provided TLS stack
- **Response caching** with configurable size limits
- **DNS resolution** through the PAL interface
- **URL parsing** with RFC 3986 compliance

### Input Layer

The input layer abstracts user interaction across different input modalities:

- **Touch input** — Single-touch and gesture recognition
- **Keyboard input** — Key events with modifier support
- **Pointer input** — Mouse and trackpad support

### Platform Abstraction Layer (PAL)

The PAL is the foundation upon which all other layers rest. It defines a set of
C functions that must be implemented for each target platform:

- Memory allocation and deallocation
- File system access (read-only)
- Monotonic timer and tick functions
- Display output (pixel buffer, resolution, color depth)
- Network socket operations
- Platform identification and capabilities

## 2.3 Data Flow

The rendering pipeline follows a well-defined sequence from URL input to
pixel output:

```
  URL Input
      |
      v
  +------------+
  |  Network   |---- HTTP Request ---->  Remote Server
  |  Stack     |<--- HTTP Response ----
  +-----+------+
        | Raw HTML/CSS bytes
        v
  +------------+
  |   HTML     |
  |   Parser   |
  +-----+------+
        | DOM Tree
        v
  +------------+    +------------+
  |   CSS      |<---|  Style     |
  |   Engine   |    |  Sheets    |
  +-----+------+    +------------+
        | Styled DOM
        v
  +------------+
  |  Layout    |
  |  Engine    |
  +-----+------+
        | Layout Tree (position, size)
        v
  +------------+
  | Rendering  |
  | Backend    |
  +-----+------+
        | Pixels
        v
  +------------+
  |  Display   |
  |  Output    |
  +------------+
```

Each stage in the pipeline is independent and testable. The HTML parser
produces a DOM tree that is consumed by the CSS engine. The CSS engine
annotates DOM nodes with computed styles that drive the layout engine.
The layout engine produces a geometry tree that the rendering backend
uses to paint pixels to the display.

## 2.4 Module Dependencies

The following table shows which modules depend on which other modules.
This dependency graph is intentionally kept shallow to minimize coupling:

| Module          | Depends On                           |
|-----------------|--------------------------------------|
| HTML Parser     | DOM Tree, PAL (memory)               |
| CSS Engine      | DOM Tree, PAL (memory)               |
| Layout Engine   | DOM Tree, CSS Engine, PAL (memory)   |
| Rendering       | Layout Engine, PAL (display)         |
| Network Stack   | PAL (network, memory)                |
| Input Layer     | PAL (input), DOM Tree                |
| JS Engine       | DOM Tree, CSS Engine (optional)      |
| CLI Interface   | Embedding API                        |
| Embedding API   | All core modules                     |

## 2.5 Project Directory Structure

```
eBrowser/
|-- src/                    # Source code
|   |-- engine/             # Core engine (parser, CSS, layout, DOM)
|   |   |-- html_parser.c   # HTML tokenizer and parser
|   |   |-- css_parser.c    # CSS stylesheet parser
|   |   |-- css_engine.c    # Style resolution and cascade
|   |   |-- layout.c        # Layout computation
|   |   +-- dom.c           # DOM tree data structure
|   |-- render/             # Rendering backend
|   |   |-- render.c        # Abstract rendering interface
|   |   |-- framebuffer.c   # Framebuffer output
|   |   +-- painter.c       # Drawing primitives
|   |-- network/            # Network stack
|   |   |-- http.c          # HTTP client
|   |   |-- tls.c           # TLS wrapper
|   |   |-- url.c           # URL parser
|   |   +-- cache.c         # Response cache
|   +-- input/              # Input handling
|       |-- input.c         # Input event dispatcher
|       |-- touch.c         # Touch input handler
|       |-- keyboard.c      # Keyboard input handler
|       +-- pointer.c       # Pointer/mouse input handler
|-- include/                # Public headers
|   +-- eBrowser/
|       |-- eBrowser.h      # Main embedding API
|       |-- eb_types.h      # Type definitions
|       |-- eb_config.h     # Configuration structures
|       +-- eb_error.h      # Error codes
|-- platform/               # Platform abstraction layer
|   |-- pal.h               # PAL interface definition
|   |-- pal_linux.c         # Linux implementation
|   |-- pal_rtos.c          # Generic RTOS implementation
|   +-- pal_baremetal.c     # Bare-metal implementation
|-- port/                   # Platform ports
|   |-- sdl2/               # SDL2 desktop port
|   |-- eos/                # EmbeddedOS native port
|   +-- web/                # Web/Emscripten port
|-- tests/                  # Test suites
|   |-- test_dom.c          # DOM tests (29 cases)
|   |-- test_html_parser.c  # HTML parser tests (27 cases)
|   |-- test_css_parser.c   # CSS parser tests (42 cases)
|   |-- test_layout.c       # Layout tests (18 cases)
|   |-- test_url.c          # URL tests (33 cases)
|   |-- test_input.c        # Input tests (16 cases)
|   +-- test_platform.c     # Platform tests (13 cases)
|-- docs/                   # Documentation
|-- assets/                 # Static assets and resources
|-- cmake/                  # CMake toolchain and module files
|   |-- eos.cmake           # EmbeddedOS toolchain file
|   +-- features.cmake      # Feature toggle definitions
|-- CMakeLists.txt          # Root build file
+-- README.md               # Project overview
```

---

# Chapter 3: The Core Rendering Engine

## 3.1 Overview

The core rendering engine is responsible for transforming raw HTML and CSS text
into a visual representation that can be painted to a display. This process
involves four major subsystems working in concert: the HTML parser, the CSS
engine, the DOM tree, and the layout engine.

The rendering pipeline operates in four distinct phases:

1. **Parsing** — HTML text is tokenized and parsed into a DOM tree
2. **Styling** — CSS rules are matched to DOM elements and styles are computed
3. **Layout** — Element positions and sizes are calculated based on styles
4. **Painting** — Layout boxes are rendered as pixels to the display

## 3.2 The HTML Parser

### 3.2.1 Parsing Model

The eBrowser HTML parser implements a streaming, event-driven parsing model
inspired by the HTML5 specification. Unlike full browser engines that maintain
complex state machines with hundreds of states, eBrowser uses a simplified
tokenizer with the following core states:

```
+----------+    '<'     +----------+   tag char   +----------+
|   DATA   |---------->| TAG_OPEN |------------>| TAG_NAME |
|  STATE   |           |  STATE   |              |  STATE   |
+----------+           +----------+              +----+-----+
     ^                                                |
     |              '>'                               | '>' or ' '
     +------------------------------------------------+
```

The parser processes input incrementally, allowing it to handle data as it
arrives from the network without buffering the entire document. This streaming
approach is critical for memory-constrained devices.

### 3.2.2 Supported HTML Elements

eBrowser supports the following HTML5 element categories:

| Category        | Elements                                                  |
|-----------------|-----------------------------------------------------------|
| Document        | `html`, `head`, `body`, `title`, `meta`, `link`          |
| Sections        | `div`, `section`, `article`, `header`, `footer`, `nav`   |
| Text Content    | `p`, `h1`-`h6`, `blockquote`, `pre`, `hr`               |
| Inline Text     | `span`, `a`, `strong`, `em`, `code`, `br`                |
| Lists           | `ul`, `ol`, `li`                                         |
| Tables          | `table`, `thead`, `tbody`, `tr`, `th`, `td`, `caption`   |
| Forms           | `form`, `input`, `button`, `select`, `option`, `textarea`|
| Media           | `img` (static images only)                                |
| Void Elements   | `br`, `hr`, `img`, `input`, `meta`, `link`               |

### 3.2.3 Parser API

```c
/* Create a new HTML parser instance */
eb_html_parser *eb_html_parser_create(eb_allocator *alloc);

/* Feed a chunk of HTML data to the parser */
eb_status eb_html_parser_feed(eb_html_parser *parser,
                               const char *data,
                               size_t length);

/* Signal end of input and finalize the DOM tree */
eb_dom_node *eb_html_parser_finish(eb_html_parser *parser);

/* Destroy the parser and free resources */
void eb_html_parser_destroy(eb_html_parser *parser);
```

### 3.2.4 Streaming Parse Example

The following example demonstrates how to parse HTML data as it arrives from
the network in chunks, rather than waiting for the entire document:

```c
eb_html_parser *parser = eb_html_parser_create(&allocator);

/* Feed data as it arrives from the network */
while ((bytes_read = eb_net_recv(conn, buffer, sizeof(buffer))) > 0) {
    eb_status status = eb_html_parser_feed(parser, buffer, bytes_read);
    if (status != EB_OK) {
        /* Handle parse error */
        eb_log(EB_LOG_ERROR, "Parse error: %s", eb_status_string(status));
        break;
    }
}

/* Finalize and get the DOM tree */
eb_dom_node *document = eb_html_parser_finish(parser);
if (document == NULL) {
    eb_log(EB_LOG_ERROR, "Failed to build DOM tree");
}

/* Use the DOM tree for layout and rendering */
eb_layout_compute(layout_ctx, document);

/* Clean up */
eb_html_parser_destroy(parser);
```

### 3.2.5 Error Recovery

The parser implements basic error recovery strategies to handle malformed HTML
gracefully, rather than failing on the first error:

1. **Unclosed tags** — Automatically closed when a sibling or parent tag is encountered
2. **Unknown tags** — Treated as generic `div`-like block elements
3. **Malformed attributes** — Best-effort parsing with graceful degradation
4. **Nested forms** — Inner forms are ignored (per HTML5 specification)
5. **Duplicate IDs** — First occurrence wins for `getElementById` lookups
6. **Missing `<html>`, `<head>`, `<body>`** — Implied elements are auto-created

## 3.3 The DOM Tree

### 3.3.1 Node Structure

The DOM tree uses a compact node representation optimized for memory efficiency.
Each node stores pointers to its parent, first child, and next sibling, forming
a tree that can be traversed in any direction:

```c
typedef struct eb_dom_node {
    eb_node_type        type;       /* Element, text, comment, document */
    eb_tag_id           tag;        /* Interned tag identifier */
    struct eb_dom_node *parent;     /* Parent node pointer */
    struct eb_dom_node *first_child;/* First child node */
    struct eb_dom_node *next_sibling;/* Next sibling node */
    eb_attr_list       *attributes; /* Attribute key-value pairs */
    eb_style           *style;      /* Computed style (after CSS resolution) */
    eb_layout_box      *layout;     /* Layout geometry (after layout pass) */
    uint16_t            flags;      /* Node state flags */
} eb_dom_node;
```

Each node occupies approximately 48 bytes on a 32-bit platform and 72 bytes on
a 64-bit platform, enabling eBrowser to represent a document with 500 elements
using less than 24 KB of memory for the DOM tree structure alone.

### 3.3.2 DOM Traversal API

```c
/* Find an element by its ID attribute */
eb_dom_node *eb_dom_get_by_id(eb_dom_node *root, const char *id);

/* Find elements by tag name (returns a node list) */
eb_node_list *eb_dom_get_by_tag(eb_dom_node *root, eb_tag_id tag);

/* Get/set attributes */
const char *eb_dom_get_attr(eb_dom_node *node, const char *name);
eb_status eb_dom_set_attr(eb_dom_node *node, const char *name,
                           const char *value);

/* Tree manipulation */
eb_status eb_dom_append_child(eb_dom_node *parent, eb_dom_node *child);
eb_status eb_dom_remove_child(eb_dom_node *parent, eb_dom_node *child);
eb_dom_node *eb_dom_create_element(eb_dom_node *doc, eb_tag_id tag);
eb_dom_node *eb_dom_create_text(eb_dom_node *doc, const char *text);

/* Tree iteration */
eb_dom_node *eb_dom_first_child(eb_dom_node *node);
eb_dom_node *eb_dom_next_sibling(eb_dom_node *node);
eb_dom_node *eb_dom_parent(eb_dom_node *node);
```

## 3.4 The CSS Engine

### 3.4.1 Supported CSS Properties

eBrowser supports the following CSS properties, covering the most commonly needed
styling capabilities for embedded user interfaces:

| Category       | Properties                                                    |
|----------------|---------------------------------------------------------------|
| Box Model      | `margin`, `padding`, `border-width`, `border-color`,          |
|                | `border-style`, `width`, `height`, `min-width`, `max-width`  |
| Typography     | `font-size`, `font-weight`, `font-family`, `color`,          |
|                | `text-align`, `text-decoration`, `line-height`               |
| Display        | `display` (block, inline, none, flex), `visibility`,          |
|                | `overflow`, `position`, `top`, `left`, `right`, `bottom`     |
| Background     | `background-color`, `background-image` (limited)              |
| Flexbox        | `flex-direction`, `justify-content`, `align-items`,           |
|                | `flex-grow`, `flex-shrink`, `flex-basis`                      |
| Units          | `px`, `em`, `rem`, `%`, `vw`, `vh`, `pt`                     |
| Colors         | Named colors, `#hex`, `rgb()`, `rgba()`                       |

### 3.4.2 Selector Support

The CSS engine supports the following selector types, listed in specificity order:

| Selector Type        | Example              | Specificity |
|----------------------|----------------------|-------------|
| Universal            | `*`                  | (0,0,0)     |
| Type                 | `div`                | (0,0,1)     |
| Class                | `.container`         | (0,1,0)     |
| ID                   | `#header`            | (1,0,0)     |
| Descendant           | `div p`              | Additive    |
| Child                | `div > p`            | Additive    |
| Attribute            | `[type="text"]`      | (0,1,0)     |
| Pseudo-class         | `:first-child`       | (0,1,0)     |
| Combined             | `div.box > p.text`   | Additive    |

### 3.4.3 Style Resolution

The CSS engine resolves styles through the standard cascade algorithm:

1. **User-agent defaults** — Built-in default styles for all elements
2. **Author stylesheets** — Styles from `<style>` tags and linked `.css` files
3. **Inline styles** — Styles from the `style` attribute on elements
4. **Specificity** — Higher specificity selectors override lower ones
5. **!important** — Overrides normal cascade order
6. **Inheritance** — Inherited properties flow down the DOM tree

```c
/* Style resolution API */
eb_status eb_css_resolve_styles(eb_css_engine *engine,
                                 eb_dom_node *document);

/* Get a computed property value */
eb_css_value eb_css_get_property(eb_dom_node *node,
                                  eb_css_property_id prop);
```

## 3.5 The Layout Engine

### 3.5.1 Layout Model

The layout engine computes the position and size of every visible element in the
document. It implements two primary layout modes and one optional mode:

**Block Layout** — Elements stack vertically, each occupying the full width of
their containing block. Block elements include `div`, `p`, `h1`-`h6`, `section`,
and others with `display: block`.

**Inline Layout** — Elements flow horizontally within a line, wrapping to the
next line when the container width is exceeded. Inline elements include `span`,
`a`, `strong`, `em`, and text content.

**Flexbox Layout** (optional) — Elements are arranged along a main axis with
flexible sizing. Enabled with `EB_ENABLE_FLEXBOX=ON`.

### 3.5.2 Box Model Implementation

Every element in the layout tree is represented as a box with four concentric
areas: margin, border, padding, and content:

```
+--------------------------------------+
|              Margin                   |
|  +--------------------------------+  |
|  |           Border               |  |
|  |  +--------------------------+  |  |
|  |  |        Padding           |  |  |
|  |  |  +------------------+   |  |  |
|  |  |  |    Content        |   |  |  |
|  |  |  |    Area           |   |  |  |
|  |  |  +------------------+   |  |  |
|  |  +--------------------------+  |  |
|  +--------------------------------+  |
+--------------------------------------+
```

### 3.5.3 Layout Computation API

```c
/* Initialize the layout context with viewport dimensions */
eb_layout_ctx *eb_layout_create(uint16_t viewport_width,
                                 uint16_t viewport_height);

/* Compute layout for the entire document */
eb_status eb_layout_compute(eb_layout_ctx *ctx,
                             eb_dom_node *document);

/* Get the layout box for a specific node */
eb_layout_box *eb_layout_get_box(eb_dom_node *node);

/* Layout box structure */
typedef struct eb_layout_box {
    int16_t x, y;           /* Position relative to parent */
    uint16_t width, height; /* Content area dimensions */
    eb_edges margin;        /* Margin edges */
    eb_edges padding;       /* Padding edges */
    eb_edges border;        /* Border widths */
} eb_layout_box;
```

### 3.5.4 Viewport and Coordinate System

eBrowser uses a coordinate system with the origin at the top-left corner of the
viewport. All coordinates are in device pixels. The viewport dimensions are
configured at initialization and can be updated dynamically:

```c
/* Set viewport dimensions */
eb_status eb_layout_set_viewport(eb_layout_ctx *ctx,
                                  uint16_t width,
                                  uint16_t height);
```

---

# Chapter 4: Platform Abstraction Layer

## 4.1 PAL Design Principles

The Platform Abstraction Layer (PAL) is the foundation of eBrowser's portability.
It defines a minimal set of C functions that encapsulate all platform-specific
behavior. By keeping this interface small and focused, porting eBrowser to a new
platform is a tractable task — typically requiring 200-500 lines of C code.

### Design Goals

1. **Minimality** — Only functions that truly vary across platforms are in the PAL
2. **Simplicity** — Each function has a clear, single responsibility
3. **Testability** — PAL implementations can be verified independently
4. **No global state** — All PAL state is passed through explicit context pointers

## 4.2 PAL Interface

### 4.2.1 Platform Detection

```c
/* Identify the current platform */
typedef enum {
    EB_PLATFORM_LINUX,
    EB_PLATFORM_RTOS,
    EB_PLATFORM_BAREMETAL,
    EB_PLATFORM_EOS,
    EB_PLATFORM_CUSTOM
} eb_platform_id;

eb_platform_id eb_pal_detect_platform(void);
const char    *eb_pal_platform_name(void);
```

### 4.2.2 Memory Management

```c
/* Allocate memory */
void *eb_pal_alloc(size_t size);

/* Reallocate memory */
void *eb_pal_realloc(void *ptr, size_t new_size);

/* Free memory */
void eb_pal_free(void *ptr);

/* Get available memory (optional, returns 0 if unsupported) */
size_t eb_pal_available_memory(void);
```

### 4.2.3 File I/O

```c
/* Open a file for reading */
eb_file *eb_pal_fopen(const char *path, const char *mode);

/* Read from a file */
size_t eb_pal_fread(void *buffer, size_t size, size_t count, eb_file *f);

/* Close a file */
void eb_pal_fclose(eb_file *f);

/* Check if a file exists */
bool eb_pal_file_exists(const char *path);
```

### 4.2.4 Timing

```c
/* Get monotonic tick count in milliseconds */
uint32_t eb_pal_tick_ms(void);

/* Sleep for a specified number of milliseconds */
void eb_pal_sleep_ms(uint32_t ms);
```

### 4.2.5 Display Output

```c
/* Initialize the display subsystem */
eb_status eb_pal_display_init(uint16_t width, uint16_t height,
                                eb_pixel_format format);

/* Get a pointer to the framebuffer */
void *eb_pal_display_get_buffer(void);

/* Flush the framebuffer to the display */
eb_status eb_pal_display_flush(uint16_t x, uint16_t y,
                                uint16_t w, uint16_t h);

/* Get display information */
eb_display_info eb_pal_display_info(void);
```

## 4.3 Porting Guide

### Step-by-Step Porting Process

To port eBrowser to a new platform, follow these steps:

1. **Create a new PAL file** — `platform/pal_<yourplatform>.c`
2. **Implement memory functions** — `eb_pal_alloc`, `eb_pal_free`, `eb_pal_realloc`
3. **Implement timing functions** — `eb_pal_tick_ms`, `eb_pal_sleep_ms`
4. **Implement file I/O** — `eb_pal_fopen`, `eb_pal_fread`, `eb_pal_fclose`
5. **Implement display output** — `eb_pal_display_init`, `eb_pal_display_flush`
6. **Register your platform** in `cmake/platforms.cmake`
7. **Run the platform test suite** — `test_platform` (13 test cases)
8. **Run the full test suite** to verify integration

### Porting Effort Estimate

| Function Category | Functions to Implement | Typical Effort |
|-------------------|----------------------|----------------|
| Memory            | 3 functions          | 1 hour         |
| Timing            | 2 functions          | 30 minutes     |
| File I/O          | 4 functions          | 1-2 hours      |
| Display           | 4 functions          | 2-4 hours      |
| Network (optional)| 5 functions          | 4-8 hours      |
| **Total**         | **13-18 functions**  | **8-16 hours** |

## 4.4 Supported Platforms

| Platform         | PAL File           | Status      | Notes                     |
|------------------|--------------------|-------------|---------------------------|
| Linux (glibc)    | `pal_linux.c`      | Complete    | Reference implementation  |
| FreeRTOS         | `pal_rtos.c`       | Complete    | Tested on ESP32, STM32    |
| Bare-metal ARM   | `pal_baremetal.c`  | Complete    | Cortex-M4/M7 tested      |
| EmbeddedOS       | `pal_eos.c`        | Complete    | Native EOS integration    |
| Zephyr RTOS      | `pal_zephyr.c`     | Beta        | Community contributed     |
| NuttX            | `pal_nuttx.c`      | Beta        | Community contributed     |

---

# Chapter 5: The Network Stack

## 5.1 Overview

The eBrowser network stack provides HTTP/1.1 client functionality with optional
TLS support for HTTPS connections. The stack is designed for minimal memory usage
and can operate with as little as 4 KB of buffer space per connection.

## 5.2 HTTP Client

### 5.2.1 Connection Management

```c
/* Create an HTTP connection */
eb_http_conn *eb_http_connect(const char *host, uint16_t port,
                               bool use_tls);

/* Send an HTTP GET request */
eb_http_response *eb_http_get(eb_http_conn *conn, const char *path,
                               eb_http_headers *headers);

/* Send an HTTP POST request */
eb_http_response *eb_http_post(eb_http_conn *conn, const char *path,
                                eb_http_headers *headers,
                                const void *body, size_t body_len);

/* Close a connection */
void eb_http_close(eb_http_conn *conn);
```

### 5.2.2 Response Processing

```c
typedef struct eb_http_response {
    uint16_t        status_code;    /* HTTP status code (200, 404, etc.) */
    eb_http_headers *headers;       /* Response headers */
    const char      *body;          /* Response body pointer */
    size_t          body_length;    /* Body length in bytes */
    const char      *content_type;  /* Content-Type header value */
    bool            chunked;        /* Transfer-Encoding: chunked */
} eb_http_response;
```

### 5.2.3 Supported HTTP Features

| Feature                   | Status        | Notes                          |
|---------------------------|---------------|--------------------------------|
| HTTP/1.1                  | Supported     | Full request/response cycle    |
| Keep-Alive                | Supported     | Connection reuse               |
| Chunked Transfer          | Supported     | Streaming response bodies      |
| Content-Length             | Supported     | Fixed-length responses         |
| Redirects (3xx)           | Supported     | Up to 5 redirect hops          |
| Basic Authentication      | Supported     | Base64-encoded credentials     |
| Gzip Decompression        | Optional      | Requires zlib                  |
| HTTP/2 [@rfc7540]                    | Not supported | Not planned for embedded use   |
| WebSockets                | Not supported | Out of scope                   |

## 5.3 TLS Support

### 5.3.1 TLS Configuration

TLS support is optional and can be enabled at build time:

```cmake
option(EB_ENABLE_TLS "Enable TLS/HTTPS support" ON)
set(EB_TLS_BACKEND "mbedtls" CACHE STRING "TLS backend (mbedtls|platform)")
```

### 5.3.2 Certificate Management

```c
/* Load CA certificates from a file */
eb_status eb_tls_load_ca_certs(eb_tls_ctx *ctx, const char *ca_file);

/* Load CA certificates from a memory buffer */
eb_status eb_tls_load_ca_certs_mem(eb_tls_ctx *ctx,
                                    const void *data, size_t len);

/* Set hostname for SNI verification */
eb_status eb_tls_set_hostname(eb_tls_ctx *ctx, const char *hostname);
```

### 5.3.3 TLS Memory Usage

| TLS Configuration       | Additional RAM  | Additional Flash |
|-------------------------|-----------------|------------------|
| mbedTLS (minimal)       | ~32 KB          | ~64 KB           |
| mbedTLS (RSA + ECC)     | ~48 KB          | ~96 KB           |
| Platform TLS            | Varies          | Minimal          |
| No TLS                  | 0 KB            | 0 KB             |

## 5.4 URL Parser

The URL parser implements RFC 3986 compliant URL parsing with full support for
the most common URL formats encountered in embedded web applications:

- **Scheme detection** — `http://`, `https://`, `file://`
- **Authority parsing** — `user:password@host:port`
- **Path normalization** — Dot-segment removal (`/../`, `/./`)
- **Query string parsing** — Key-value pair extraction
- **Fragment handling** — Hash fragment identification
- **Relative URL resolution** — Resolving relative URLs against a base URL

```c
typedef struct eb_url {
    char scheme[8];         /* "http" or "https" */
    char host[256];         /* Hostname */
    uint16_t port;          /* Port number (0 = default) */
    char path[1024];        /* Request path */
    char query[512];        /* Query string */
    char fragment[128];     /* Fragment identifier */
} eb_url;

eb_status eb_url_parse(const char *url_string, eb_url *result);
eb_status eb_url_resolve(const eb_url *base, const char *relative,
                          eb_url *result);
```

## 5.5 Response Caching

The network stack includes a configurable response cache that reduces network
traffic and improves page load times for repeated visits:

```c
/* Configure cache size (in bytes) */
eb_status eb_cache_set_size(eb_cache *cache, size_t max_bytes);

/* Clear the cache */
eb_status eb_cache_clear(eb_cache *cache);

/* Cache statistics */
typedef struct eb_cache_stats {
    uint32_t hits;          /* Cache hit count */
    uint32_t misses;        /* Cache miss count */
    size_t   bytes_used;    /* Current cache size */
    size_t   bytes_max;     /* Maximum cache size */
    uint16_t entries;       /* Number of cached entries */
} eb_cache_stats;

eb_cache_stats eb_cache_get_stats(eb_cache *cache);
```

---

# Chapter 6: Input Handling

## 6.1 Input Architecture

The eBrowser input layer provides a unified event model that abstracts across
different input modalities. The architecture follows an event-driven pattern
where input events are captured by platform-specific handlers, translated into
a common event format, and dispatched to the appropriate DOM elements.

```
+---------------+  +---------------+  +---------------+
|  Touch Panel  |  |   Keyboard    |  |  Mouse/Pad    |
+-------+-------+  +-------+-------+  +-------+-------+
        |                  |                  |
        v                  v                  v
+-------------------------------------------------------+
|              Platform Input Drivers (PAL)               |
+---------------------------+---------------------------+
                            |
                            v
+-------------------------------------------------------+
|              Input Event Dispatcher                    |
|  +---------------------------------------------------+|
|  |  Event Queue  [ ev1 | ev2 | ev3 | ... | evN ]    ||
|  +---------------------------------------------------+|
+---------------------------+---------------------------+
                            |
                            v
+-------------------------------------------------------+
|              Hit Testing & DOM Dispatch                 |
+-------------------------------------------------------+
```

## 6.2 Event Types

```c
typedef enum {
    EB_EVENT_POINTER_DOWN,   /* Mouse button or touch start */
    EB_EVENT_POINTER_UP,     /* Mouse button or touch end */
    EB_EVENT_POINTER_MOVE,   /* Mouse move or touch drag */
    EB_EVENT_KEY_DOWN,       /* Key pressed */
    EB_EVENT_KEY_UP,         /* Key released */
    EB_EVENT_KEY_CHAR,       /* Character input (after key mapping) */
    EB_EVENT_TOUCH_START,    /* Touch started */
    EB_EVENT_TOUCH_MOVE,     /* Touch moved */
    EB_EVENT_TOUCH_END,      /* Touch ended */
    EB_EVENT_SCROLL,         /* Scroll wheel or gesture */
    EB_EVENT_FOCUS,          /* Element gained focus */
    EB_EVENT_BLUR,           /* Element lost focus */
} eb_event_type;
```

## 6.3 Event Structure

```c
typedef struct eb_input_event {
    eb_event_type type;     /* Event type */
    uint32_t      timestamp;/* Event timestamp (ms) */
    union {
        struct {
            int16_t x, y;   /* Pointer position */
            uint8_t button;  /* Button index */
        } pointer;
        struct {
            uint16_t keycode;/* Key code */
            uint16_t modifiers;/* Modifier flags */
            char     character;/* Mapped character */
        } key;
        struct {
            int16_t x, y;   /* Touch position */
            uint8_t id;      /* Touch point ID */
        } touch;
        struct {
            int16_t dx, dy;  /* Scroll delta */
        } scroll;
    };
} eb_input_event;
```

## 6.4 Input Callbacks

Applications can register callbacks to intercept input events before they
reach the DOM dispatching stage:

```c
/* Register a global input callback */
typedef bool (*eb_input_callback)(eb_instance *browser,
                                   const eb_input_event *event,
                                   void *user_data);

eb_status eb_set_input_callback(eb_instance *browser,
                                 eb_input_callback callback,
                                 void *user_data);
```

The callback returns `true` to consume the event (preventing DOM dispatch) or
`false` to allow normal processing to continue.

## 6.5 Touch Input

### Touch Gesture Recognition

eBrowser includes built-in gesture recognition for common touch interactions:

| Gesture       | Detection Criteria                               | Action           |
|---------------|--------------------------------------------------|------------------|
| Tap           | Touch < 300 ms, movement < 10 px                | Click            |
| Long Press    | Touch > 500 ms, movement < 10 px                | Context action   |
| Scroll        | Touch movement > 10 px in one axis               | Page scroll      |
| Swipe         | Fast movement > 50 px in < 200 ms               | Navigation       |

## 6.6 Keyboard Input

### Key Modifier Flags

```c
#define EB_MOD_SHIFT   (1 << 0)
#define EB_MOD_CTRL    (1 << 1)
#define EB_MOD_ALT     (1 << 2)
#define EB_MOD_META    (1 << 3)
```

### Focus Management

eBrowser implements a simple focus model for form elements. The Tab key
advances focus to the next focusable element, and Shift+Tab moves to
the previous element. Focusable elements include `input`, `textarea`,
`select`, `button`, and `a` (anchor) elements.

---

# Chapter 7: Building and Configuration

## 7.1 Prerequisites

eBrowser uses CMake as its build system and supports the following toolchains:

| Requirement      | Minimum Version | Recommended          |
|------------------|-----------------|----------------------|
| C Compiler       | GCC 9 / Clang 11| GCC 12 / Clang 15  |
| C++ Compiler     | GCC 9 / Clang 11| GCC 12 / Clang 15  |
| CMake            | 3.16            | 3.25+                |
| SDL2 (optional)  | 2.0.12          | 2.28+                |
| LVGL (optional)  | 9.2             | 9.2                  |

### Zero-Prerequisites Quick Start

eBrowser provides setup scripts that auto-detect your operating system and install
all required dependencies automatically:

```bash
# Clone and enter the repository
>>> git clone https://github.com/embeddedos-org/eBrowser.git
>>> cd eBrowser

# Run the setup script (auto-detects OS, installs GCC/Clang, CMake, SDL2, LVGL)
>>> ./setup.sh

# Build and launch with default 800x480 GUI window
>>> mkdir build && cd build
>>> cmake ..
>>> make -j$(nproc)
>>> ./eBrowser
```

## 7.2 CMake Build Options

### Core Feature Toggles

| CMake Option                | Type   | Default | Description                          |
|-----------------------------|--------|---------|--------------------------------------|
| `EB_ENABLE_JS`              | BOOL   | OFF     | Enable JavaScript engine             |
| `EB_ENABLE_TLS`             | BOOL   | ON      | Enable TLS/HTTPS support             |
| `EB_ENABLE_IMAGES`          | BOOL   | ON      | Enable image decoding support        |
| `EB_ENABLE_TOUCH`           | BOOL   | ON      | Enable touch input support           |
| `EB_ENABLE_KEYBOARD`        | BOOL   | ON      | Enable keyboard input support        |
| `EB_ENABLE_TABLES`          | BOOL   | ON      | Enable HTML table layout             |
| `EB_ENABLE_FLEXBOX`         | BOOL   | OFF     | Enable CSS flexbox layout            |
| `EB_ENABLE_CACHE`           | BOOL   | ON      | Enable response caching              |

### Display Backend Selection

| CMake Option                | Type   | Default | Description                          |
|-----------------------------|--------|---------|--------------------------------------|
| `EB_DISPLAY_SDL2`           | BOOL   | ON      | Build SDL2 display backend           |
| `EB_DISPLAY_LVGL`           | BOOL   | OFF     | Build LVGL display backend           |
| `EB_DISPLAY_FRAMEBUFFER`    | BOOL   | OFF     | Build framebuffer display backend    |

### Memory Configuration

| CMake Option                | Type   | Default | Description                          |
|-----------------------------|--------|---------|--------------------------------------|
| `EB_DOM_POOL_SIZE`          | INT    | 1024    | Maximum DOM nodes in pool            |
| `EB_STYLE_POOL_SIZE`        | INT    | 512     | Maximum style objects in pool        |
| `EB_LAYOUT_POOL_SIZE`       | INT    | 512     | Maximum layout boxes in pool         |
| `EB_STRING_INTERN_SIZE`     | INT    | 256     | String interning table size          |
| `EB_NET_BUFFER_SIZE`        | INT    | 4096    | Network receive buffer size (bytes)  |
| `EB_CACHE_DEFAULT_SIZE`     | INT    | 65536   | Default cache size (bytes)           |

### Build Type Configuration

```bash
# Debug build (with assertions and debug symbols)
>>> cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build (optimized, no debug symbols)
>>> cmake -DCMAKE_BUILD_TYPE=Release ..

# MinSizeRel (optimized for size - recommended for embedded targets)
>>> cmake -DCMAKE_BUILD_TYPE=MinSizeRel ..

# Custom minimal build (no JS, no TLS, no images, framebuffer only)
>>> cmake -DCMAKE_BUILD_TYPE=MinSizeRel \
          -DEB_ENABLE_JS=OFF \
          -DEB_ENABLE_TLS=OFF \
          -DEB_ENABLE_IMAGES=OFF \
          -DEB_DISPLAY_SDL2=OFF \
          -DEB_DISPLAY_FRAMEBUFFER=ON ..
```

## 7.3 Cross-Compilation

### Using Toolchain Files

Cross-compilation is supported through CMake toolchain files. eBrowser ships
with a toolchain file for EmbeddedOS:

```bash
# Cross-compile for EmbeddedOS
>>> cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/eos.cmake ..
>>> make -j$(nproc)
```

### Writing a Custom Toolchain File

```cmake
# cmake/my_target.cmake - Toolchain file for custom ARM target

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Specify the cross-compiler
set(CMAKE_C_COMPILER   arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

# Target-specific flags
set(CMAKE_C_FLAGS_INIT   "-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16")
set(CMAKE_CXX_FLAGS_INIT "-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16")

# Linker settings
set(CMAKE_EXE_LINKER_FLAGS_INIT "-T ${CMAKE_SOURCE_DIR}/linker/target.ld")

# Where to look for libraries and headers
set(CMAKE_FIND_ROOT_PATH /opt/arm-toolchain/arm-none-eabi)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

## 7.4 Build Size Analysis

The following table shows typical binary sizes for different build configurations
across multiple target architectures:

| Configuration                  | ARM Cortex-M7 | ARM Cortex-A53 | x86_64 Linux |
|--------------------------------|---------------|----------------|--------------|
| Minimal (HTML + CSS only)      | 48 KB         | 62 KB          | 78 KB        |
| Standard (+ images + cache)    | 96 KB         | 124 KB         | 156 KB       |
| Full (+ JS + TLS + all inputs) | 280 KB        | 340 KB         | 420 KB       |
| Full + LVGL display            | 380 KB        | 450 KB         | 540 KB       |
| Full + SDL2 display            | N/A           | 360 KB         | 440 KB       |

---

# Chapter 8: The Embedding API

## 8.1 API Overview

The eBrowser embedding API is designed for simplicity. A minimal integration
requires just five function calls:

```c
#include "eBrowser/eBrowser.h"

int main(void) {
    /* 1. Create a default configuration */
    eb_config cfg = eb_config_default();

    /* 2. Create a browser instance */
    eb_instance *browser = eb_create(&cfg);
    if (!browser) {
        return -1;
    }

    /* 3. Navigate to a URL */
    eb_navigate(browser, "https://example.com");

    /* 4. Enter the event loop */
    eb_run(browser);

    /* 5. Clean up */
    eb_destroy(browser);
    return 0;
}
```

## 8.2 Configuration

### 8.2.1 Configuration Structure

```c
typedef struct eb_config {
    /* Display settings */
    uint16_t        width;          /* Viewport width in pixels */
    uint16_t        height;         /* Viewport height in pixels */
    eb_pixel_format pixel_format;   /* Pixel format (RGB565, RGB888, etc.) */
    bool            fullscreen;     /* Start in fullscreen mode */

    /* Feature flags */
    bool            enable_js;      /* Enable JavaScript engine */
    bool            enable_tls;     /* Enable TLS support */
    bool            enable_cache;   /* Enable response cache */
    bool            enable_images;  /* Enable image decoding */

    /* Memory limits */
    size_t          cache_size;     /* Maximum cache size in bytes */
    uint16_t        max_dom_nodes;  /* Maximum DOM nodes */
    uint16_t        max_connections;/* Maximum simultaneous connections */

    /* Logging */
    eb_log_level    log_level;      /* Logging verbosity */
    eb_log_callback log_callback;   /* Custom log handler */
    void           *log_user_data;  /* User data for log callback */

    /* Callbacks */
    eb_navigate_callback on_navigate;   /* Navigation event handler */
    eb_error_callback    on_error;      /* Error event handler */
    void                *callback_data; /* User data for callbacks */
} eb_config;
```

### 8.2.2 Default Configuration Values

```c
eb_config eb_config_default(void) {
    eb_config cfg = {
        .width          = 800,
        .height         = 480,
        .pixel_format   = EB_PIXEL_RGB565,
        .fullscreen     = false,
        .enable_js      = false,
        .enable_tls     = true,
        .enable_cache   = true,
        .enable_images  = true,
        .cache_size     = 64 * 1024,    /* 64 KB */
        .max_dom_nodes  = 1024,
        .max_connections= 4,
        .log_level      = EB_LOG_WARN,
        .log_callback   = NULL,
        .log_user_data  = NULL,
        .on_navigate    = NULL,
        .on_error       = NULL,
        .callback_data  = NULL,
    };
    return cfg;
}
```

## 8.3 Lifecycle Management

### 8.3.1 Instance Lifecycle

```
  eb_config_default()
         |
         v
  +--------------+
  |  eb_create() |---- Allocates resources, initializes subsystems
  +------+-------+
         |
         v
  +--------------+
  | eb_navigate()|---- Fetches and parses the document
  +------+-------+
         |
         v
  +--------------+
  |   eb_run()   |---- Enters event loop (blocks until exit)
  +------+-------+
         |
         v
  +--------------+
  | eb_destroy() |---- Frees all resources
  +--------------+
```

### 8.3.2 Non-Blocking Event Loop

For applications that need to integrate eBrowser into an existing event loop,
the non-blocking polling API allows interleaving eBrowser processing with
application logic:

```c
eb_instance *browser = eb_create(&cfg);
eb_navigate(browser, "https://example.com");

/* Non-blocking event processing */
while (app_running) {
    /* Process eBrowser events (non-blocking, 16 ms timeout) */
    eb_status status = eb_poll(browser, 16);

    /* Process your application events */
    app_process_events();

    /* Render if needed */
    if (eb_needs_repaint(browser)) {
        eb_repaint(browser);
    }
}

eb_destroy(browser);
```

## 8.4 Navigation API

```c
/* Navigate to a URL */
eb_status eb_navigate(eb_instance *browser, const char *url);

/* Navigate back in history */
eb_status eb_navigate_back(eb_instance *browser);

/* Navigate forward in history */
eb_status eb_navigate_forward(eb_instance *browser);

/* Reload the current page */
eb_status eb_reload(eb_instance *browser);

/* Stop loading the current page */
eb_status eb_stop(eb_instance *browser);

/* Get the current URL */
const char *eb_get_url(eb_instance *browser);

/* Get the page title */
const char *eb_get_title(eb_instance *browser);

/* Check loading status */
bool eb_is_loading(eb_instance *browser);
```

## 8.5 DOM Access API

```c
/* Get the document root node */
eb_dom_node *eb_get_document(eb_instance *browser);

/* Find element by ID */
eb_dom_node *eb_find_element_by_id(eb_instance *browser, const char *id);

/* Execute a simple CSS selector query */
eb_dom_node *eb_query_selector(eb_instance *browser, const char *selector);

/* Get inner text of an element */
const char *eb_get_inner_text(eb_dom_node *node);

/* Set inner HTML of an element */
eb_status eb_set_inner_html(eb_dom_node *node, const char *html);
```

## 8.6 Error Handling

```c
typedef enum {
    EB_OK = 0,                  /* Success */
    EB_ERR_NOMEM,               /* Out of memory */
    EB_ERR_INVALID_ARG,         /* Invalid argument */
    EB_ERR_NETWORK,             /* Network error */
    EB_ERR_DNS,                 /* DNS resolution failed */
    EB_ERR_TLS,                 /* TLS handshake failed */
    EB_ERR_PARSE,               /* HTML/CSS parse error */
    EB_ERR_LAYOUT,              /* Layout computation error */
    EB_ERR_RENDER,              /* Rendering error */
    EB_ERR_NOT_FOUND,           /* Resource not found (404) */
    EB_ERR_TIMEOUT,             /* Operation timed out */
    EB_ERR_UNSUPPORTED,         /* Feature not supported/enabled */
    EB_ERR_PLATFORM,            /* Platform-specific error */
} eb_status;

/* Get a human-readable error description */
const char *eb_status_string(eb_status status);
```

## 8.7 Advanced: Custom Rendering Target

For specialized display hardware that does not fit the standard framebuffer
model, eBrowser supports custom rendering callbacks:

```c
/* Set a custom rendering callback instead of using a display backend */
typedef void (*eb_render_callback)(const eb_render_cmd *cmd,
                                    void *user_data);

eb_status eb_set_render_callback(eb_instance *browser,
                                  eb_render_callback callback,
                                  void *user_data);

/* Render command types */
typedef enum {
    EB_RENDER_FILL_RECT,    /* Fill a rectangle with a color */
    EB_RENDER_DRAW_TEXT,    /* Draw text at a position */
    EB_RENDER_DRAW_BORDER,  /* Draw a border rectangle */
    EB_RENDER_DRAW_IMAGE,   /* Draw an image (if enabled) */
    EB_RENDER_CLIP_RECT,    /* Set clipping rectangle */
} eb_render_cmd_type;
```

---

# Chapter 9: JavaScript Engine

## 9.1 Overview

The eBrowser JavaScript engine is an optional component that provides basic
scripting support for embedded applications. It is deliberately minimal — focused
on DOM manipulation and event handling rather than full ECMAScript compliance.

### Design Decisions

| Decision                    | Choice              | Rationale                     |
|-----------------------------|---------------------|-------------------------------|
| JS Standard                | ES5 subset          | Smallest viable feature set   |
| Memory model               | Stack-based VM      | Predictable memory usage      |
| Garbage collection          | Reference counting  | Low-latency, deterministic    |
| JIT compilation             | Not supported       | Saves code size and RAM       |
| eval() support              | Not supported       | Security and complexity       |
| Async operations            | setTimeout only     | Minimal event loop support    |

## 9.2 Enabling JavaScript

```cmake
# Enable JS at build time
>>> cmake -DEB_ENABLE_JS=ON ..
```

```c
/* Enable JS at runtime */
eb_config cfg = eb_config_default();
cfg.enable_js = true;
eb_instance *browser = eb_create(&cfg);
```

## 9.3 Supported JavaScript Features

### 9.3.1 Language Features

| Feature Category     | Supported                                         |
|----------------------|---------------------------------------------------|
| Variables            | `var`, `let`, `const`                             |
| Data types           | Number, String, Boolean, null, undefined, Object  |
| Operators            | Arithmetic, comparison, logical, assignment       |
| Control flow         | `if/else`, `for`, `while`, `do/while`, `switch`  |
| Functions            | Declaration, expression, arrow (limited)          |
| Objects              | Literals, property access, `this` binding         |
| Arrays               | Literals, `push`, `pop`, `length`, `forEach`     |
| Strings              | Concatenation, `length`, `indexOf`, `substring`  |
| Error handling       | `try/catch/finally`                               |
| Type conversion      | Implicit and explicit coercion                    |

### 9.3.2 DOM API Bindings

```javascript
// Supported DOM APIs in eBrowser JavaScript
document.getElementById("myId");
document.querySelector(".myClass");
document.createElement("div");

element.innerHTML = "<p>Hello</p>";
element.textContent = "Hello";
element.setAttribute("class", "active");
element.getAttribute("class");
element.classList.add("highlight");
element.classList.remove("highlight");
element.style.color = "red";
element.style.display = "none";

element.appendChild(child);
element.removeChild(child);
```

### 9.3.3 Event API Bindings

```javascript
// Event listeners
element.addEventListener("click", function(event) {
    event.preventDefault();
    // Handle click
});

element.removeEventListener("click", handler);

// Timer functions
var id = setTimeout(function() {
    // Delayed action
}, 1000);

clearTimeout(id);

var intervalId = setInterval(function() {
    // Repeated action
}, 500);

clearInterval(intervalId);
```

## 9.4 JavaScript Memory Management

The JS engine uses reference counting for memory management, which provides
deterministic cleanup without garbage collection pauses — critical for
real-time embedded systems:

| JS Configuration           | Additional RAM | Additional Flash |
|----------------------------|---------------|------------------|
| JS engine (minimal)        | ~16 KB        | ~48 KB           |
| JS engine + DOM bindings   | ~24 KB        | ~64 KB           |
| Per-script overhead        | ~2-4 KB       | N/A              |

## 9.5 Performance Characteristics

The JS engine is designed for short, event-driven scripts rather than
compute-intensive workloads:

| Operation                   | Approximate Performance     |
|-----------------------------|-----------------------------|
| Simple DOM query            | < 0.1 ms                    |
| DOM tree traversal (100)    | < 1 ms                      |
| Style modification          | < 0.1 ms                    |
| Event handler execution     | < 0.5 ms                    |
| Script parse + compile      | ~1 ms per KB of script      |

## 9.6 Scripting Limitations

The following JavaScript features are **not supported** in eBrowser:

- `eval()` and `new Function()` — Dynamic code execution
- Regular expressions — Full regex engine is too large
- `Promise`, `async/await` — Asynchronous programming patterns
- `fetch()` — Network requests from JS
- Web Workers — Multi-threaded JavaScript
- Module imports (`import`/`export`) — Module system
- Prototype chain manipulation — `__proto__`, `Object.create()`
- Generators and iterators — `function*`, `yield`

---

# Chapter 10: Display and Rendering Backend

## 10.1 Rendering Architecture

The rendering backend translates the layout tree into visual output. It uses a
command-based architecture where the layout engine generates a list of rendering
commands that the display backend executes:

```
+---------------------------------------+
|           Layout Tree                  |
|  +------+ +------+ +------+           |
|  | Box  | | Box  | | Box  | ...       |
|  +--+---+ +--+---+ +--+---+           |
|     +--------+--------+               |
|              |                         |
|              v                         |
|     +----------------+                 |
|     |  Paint Tree    |                 |
|     |  Generator     |                 |
|     +--------+-------+                 |
|              |                         |
|              v                         |
|     +----------------+                 |
|     |  Render        |                 |
|     |  Command List  |                 |
|     +--------+-------+                 |
+--------------|--------------------------+
               |
               v
+--------------------------------------+
|         Display Backend               |
|  +---------+----------+------------+ |
|  |  SDL2   |  LVGL    |Framebuffer | |
|  |  Port   |  Port    |  Direct    | |
|  +---------+----------+------------+ |
+--------------------------------------+
```

## 10.2 Drawing Primitives

The rendering backend provides the following drawing primitives:

```c
/* Fill a rectangle with a solid color */
void eb_render_fill_rect(eb_render_ctx *ctx,
                          int16_t x, int16_t y,
                          uint16_t w, uint16_t h,
                          eb_color color);

/* Draw text at a specified position */
void eb_render_draw_text(eb_render_ctx *ctx,
                          int16_t x, int16_t y,
                          const char *text, size_t len,
                          eb_font *font, eb_color color);

/* Draw a border (rectangle outline) */
void eb_render_draw_border(eb_render_ctx *ctx,
                            int16_t x, int16_t y,
                            uint16_t w, uint16_t h,
                            eb_edges widths,
                            eb_color colors[4]);

/* Draw an image (if image support is enabled) */
void eb_render_draw_image(eb_render_ctx *ctx,
                           int16_t x, int16_t y,
                           uint16_t w, uint16_t h,
                           const eb_image *image);

/* Set a clipping rectangle */
void eb_render_set_clip(eb_render_ctx *ctx,
                         int16_t x, int16_t y,
                         uint16_t w, uint16_t h);
```

## 10.3 SDL2 Display Port

The SDL2 port is the primary development and testing backend. It creates a
desktop window that simulates the embedded display.

| Feature                | Status       | Notes                          |
|------------------------|-------------|--------------------------------|
| Window creation        | Supported   | Resizable or fixed size        |
| Hardware acceleration  | Supported   | GPU-accelerated rendering      |
| Fullscreen mode        | Supported   | Toggle with --fullscreen       |
| Mouse input            | Supported   | Click, move, scroll            |
| Keyboard input         | Supported   | Full key event support         |
| Touch simulation       | Supported   | Mouse acts as single touch     |
| High DPI               | Supported   | Auto-detects display scaling   |

## 10.4 LVGL Integration

eBrowser can use LVGL v9.2 as its display backend, making it suitable for
devices that already use LVGL for their UI framework:

```cmake
# Enable LVGL display backend
>>> cmake -DEB_DISPLAY_LVGL=ON \
          -DEB_DISPLAY_SDL2=OFF \
          -DLVGL_DIR=/path/to/lvgl ..
```

```c
/* eBrowser renders to an LVGL canvas widget */
lv_obj_t *canvas = lv_canvas_create(lv_scr_act());
lv_canvas_set_buffer(canvas, buffer, 800, 480, LV_COLOR_FORMAT_RGB565);

/* Pass the canvas buffer to eBrowser */
eb_config cfg = eb_config_default();
cfg.width = 800;
cfg.height = 480;
cfg.pixel_format = EB_PIXEL_RGB565;

eb_instance *browser = eb_create(&cfg);
eb_set_render_target(browser, lv_canvas_get_buf(canvas));
```

## 10.5 Framebuffer Direct Output

For bare-metal and RTOS targets, eBrowser can render directly to a
memory-mapped framebuffer:

```c
/* Get the hardware framebuffer address */
uint16_t *fb = (uint16_t *)0x20000000; /* Example: SRAM framebuffer */

eb_config cfg = eb_config_default();
cfg.width = 480;
cfg.height = 320;
cfg.pixel_format = EB_PIXEL_RGB565;

eb_instance *browser = eb_create(&cfg);
eb_set_render_target(browser, fb);
```

## 10.6 Pixel Formats

| Format             | Bits/Pixel | Use Case                         |
|--------------------|------------|----------------------------------|
| `EB_PIXEL_RGB565`  | 16         | Most embedded displays           |
| `EB_PIXEL_RGB888`  | 24         | Higher-quality displays          |
| `EB_PIXEL_ARGB8888`| 32         | Desktop/transparency support     |
| `EB_PIXEL_MONO`    | 1          | Monochrome/e-ink displays        |
| `EB_PIXEL_GRAY8`   | 8          | Grayscale displays               |

## 10.7 Font Rendering

eBrowser includes a built-in bitmap font renderer that supports:

- **Built-in font** — A compact, embedded-friendly sans-serif font
- **Custom fonts** — Load additional fonts from `.ebfont` format files
- **Font sizes** — Discrete sizes: 8, 10, 12, 14, 16, 20, 24 pixels
- **UTF-8 support** — Basic Latin and Latin-1 Supplement character ranges

```c
/* Load a custom font */
eb_font *font = eb_font_load("/assets/custom_12px.ebfont");

/* Set the default font */
eb_set_default_font(browser, font);
```

---

# Chapter 11: Testing and Quality Assurance

## 11.1 Test Suite Overview

eBrowser includes a comprehensive test suite organized into 7 test modules with
over 130 individual test cases. The tests are designed to validate every major
subsystem independently and can run on both host development machines and
embedded targets.

### Test Suite Summary

| Test Module        | File                  | Test Cases | Coverage Area              |
|--------------------|-----------------------|------------|----------------------------|
| `test_dom`         | `tests/test_dom.c`    | 29         | DOM tree operations         |
| `test_html_parser` | `tests/test_html_parser.c` | 27    | HTML tokenization & parsing |
| `test_css_parser`  | `tests/test_css_parser.c`  | 42    | CSS parsing & resolution    |
| `test_layout`      | `tests/test_layout.c` | 18         | Layout computation          |
| `test_url`         | `tests/test_url.c`    | 33         | URL parsing & resolution    |
| `test_input`       | `tests/test_input.c`  | 16         | Input event handling        |
| `test_platform`    | `tests/test_platform.c`| 13        | PAL implementation          |
| **Total**          |                       | **178**    |                             |

## 11.2 Running Tests

### Build and Run All Tests

```bash
# Build with tests enabled
>>> cd build
>>> cmake -DEB_BUILD_TESTS=ON ..
>>> make -j$(nproc)

# Run all tests
>>> ctest --output-on-failure

# Run a specific test suite
>>> ./tests/test_dom
>>> ./tests/test_html_parser
>>> ./tests/test_css_parser
```

### Test Output Format

```
[==========] Running 29 tests from test_dom
[ RUN      ] test_dom_create_document
[       OK ] test_dom_create_document (0 ms)
[ RUN      ] test_dom_create_element
[       OK ] test_dom_create_element (0 ms)
[ RUN      ] test_dom_append_child
[       OK ] test_dom_append_child (0 ms)
...
[==========] 29 tests from test_dom ran (2 ms total)
[  PASSED  ] 29 tests
[  FAILED  ] 0 tests
```

## 11.3 Test Suite Details

### 11.3.1 DOM Tests (test_dom - 29 cases)

| Test Category           | Tests | Description                              |
|-------------------------|-------|------------------------------------------|
| Tree creation           | 5     | Document, element, and text node creation|
| Attribute handling      | 6     | Get, set, remove, enumerate attributes   |
| Tree manipulation       | 7     | Append, remove, insert, replace children |
| Tree traversal          | 5     | Parent, child, sibling navigation        |
| Search operations       | 4     | getElementById, getElementsByTag         |
| Memory management       | 2     | Pool allocation, cleanup                 |

### 11.3.2 HTML Parser Tests (test_html_parser - 27 cases)

| Test Category           | Tests | Description                              |
|-------------------------|-------|------------------------------------------|
| Tokenizer               | 6     | Tag open/close, attributes, text content |
| Void elements           | 4     | `br`, `hr`, `img`, `input` handling      |
| HTML5 elements          | 5     | Semantic elements, nesting rules         |
| Form elements           | 4     | `form`, `input`, `select`, `textarea`    |
| Table elements          | 4     | `table`, `tr`, `td`, implicit close      |
| Error recovery          | 4     | Malformed HTML, unclosed tags            |

### 11.3.3 CSS Parser Tests (test_css_parser - 42 cases)

| Test Category           | Tests | Description                              |
|-------------------------|-------|------------------------------------------|
| Color values            | 8     | Named, hex, rgb(), rgba()               |
| Length values           | 6     | px, em, rem, %, vw, vh                  |
| Selectors              | 10    | Type, class, ID, descendant, child       |
| Property parsing        | 8     | Box model, typography, display           |
| Style resolution        | 6     | Cascade, specificity, inheritance        |
| Shorthand properties    | 4     | `margin`, `padding`, `border`           |

### 11.3.4 Layout Tests (test_layout - 18 cases)

| Test Category           | Tests | Description                              |
|-------------------------|-------|------------------------------------------|
| Viewport initialization | 3     | Viewport setup, resize, DPI              |
| Box building            | 4     | Content, padding, border, margin boxes   |
| Block layout            | 5     | Vertical stacking, width calculation     |
| Inline layout           | 4     | Horizontal flow, line wrapping           |
| Nested layout           | 2     | Block-in-block, inline-in-block         |

### 11.3.5 URL Tests (test_url - 33 cases)

| Test Category           | Tests | Description                              |
|-------------------------|-------|------------------------------------------|
| Basic URL parsing       | 8     | Scheme, host, port, path extraction      |
| Default ports           | 4     | HTTP 80, HTTPS 443, missing port         |
| Path handling           | 5     | Normalization, dot segments, encoding    |
| Relative resolution     | 8     | Resolve against base URL                 |
| Real-world URLs         | 5     | Complex, edge-case URLs from the wild    |
| Error handling          | 3     | Invalid URLs, empty inputs               |

### 11.3.6 Input Tests (test_input - 16 cases)

| Test Category           | Tests | Description                              |
|-------------------------|-------|------------------------------------------|
| Pointer events          | 4     | Mouse down, up, move, click sequence     |
| Keyboard events         | 4     | Key down, up, character mapping          |
| Touch events            | 4     | Touch start, move, end, gesture          |
| Callback registration   | 4     | Register, invoke, remove callbacks       |

### 11.3.7 Platform Tests (test_platform - 13 cases)

| Test Category           | Tests | Description                              |
|-------------------------|-------|------------------------------------------|
| Platform detection      | 2     | Identify OS, get platform name           |
| Memory allocation       | 4     | Alloc, realloc, free, out-of-memory      |
| File I/O                | 4     | Open, read, close, existence check       |
| Timing                  | 3     | Tick resolution, sleep accuracy          |

## 11.4 CI/CD Integration

eBrowser is tested automatically on every commit using a CI/CD pipeline:

| CI Stage              | Description                                      |
|-----------------------|--------------------------------------------------|
| Build (Debug)         | Compile with all warnings, debug assertions      |
| Build (Release)       | Compile with optimizations enabled               |
| Build (MinSizeRel)    | Compile for minimum binary size                  |
| Unit Tests            | Run all 7 test suites on host                    |
| Cross-Compile         | Build for ARM Cortex-M7 and Cortex-A53           |
| Static Analysis       | Run cppcheck and clang-tidy                      |
| Memory Sanitizer      | Run tests under ASan and MSan                    |
| Size Regression       | Check that binary size stays within budget        |

## 11.5 Code Quality Standards

| Metric                | Target          | Current     |
|-----------------------|-----------------|-------------|
| Test coverage (line)  | > 80%           | 84%         |
| Test coverage (branch)| > 70%           | 73%         |
| Static analysis issues| 0 critical      | 0 critical  |
| Compiler warnings     | 0 with -Wall    | 0           |
| Memory leaks (ASan)   | 0               | 0           |

---

# Chapter 12: Performance Optimization

## 12.1 Memory Optimization Strategies

### 12.1.1 Pool-Based Allocation

eBrowser uses memory pools for frequently allocated objects to reduce
fragmentation and allocation overhead:

```c
/* DOM node pool - pre-allocated array of node structures */
typedef struct eb_dom_pool {
    eb_dom_node  nodes[EB_DOM_POOL_SIZE];
    uint16_t     free_list[EB_DOM_POOL_SIZE];
    uint16_t     free_count;
    uint16_t     total_allocated;
} eb_dom_pool;

/* Allocate a node from the pool (O(1) operation) */
eb_dom_node *eb_dom_pool_alloc(eb_dom_pool *pool);

/* Return a node to the pool */
void eb_dom_pool_free(eb_dom_pool *pool, eb_dom_node *node);
```

### 12.1.2 String Interning

String interning deduplicates commonly used strings such as tag names,
attribute names, and CSS property names:

```c
/* String intern table */
typedef struct eb_intern_table {
    const char *strings[EB_STRING_INTERN_SIZE];
    uint32_t    hashes[EB_STRING_INTERN_SIZE];
    uint16_t    count;
} eb_intern_table;

/* Intern a string - returns a pointer to the shared copy */
const char *eb_intern_string(eb_intern_table *table, const char *str);
```

### 12.1.3 Memory Usage by Component

| Component            | Minimum RAM | Typical RAM | Maximum RAM |
|----------------------|-------------|-------------|-------------|
| DOM tree (100 nodes) | 4.8 KB      | 8 KB        | 16 KB       |
| CSS styles           | 2 KB        | 6 KB        | 12 KB       |
| Layout boxes         | 1.8 KB      | 4 KB        | 8 KB        |
| String intern table  | 1 KB        | 2 KB        | 4 KB        |
| Network buffers      | 4 KB        | 8 KB        | 16 KB       |
| Response cache       | 0 KB        | 64 KB       | 256 KB      |
| JS engine (optional) | 16 KB       | 24 KB       | 48 KB       |
| **Total (no JS)**    | **13.6 KB** | **92 KB**   | **312 KB**  |
| **Total (with JS)**  | **29.6 KB** | **116 KB**  | **360 KB**  |

## 12.2 Binary Size Optimization

### Compiler Flags for Size

```cmake
# MinSizeRel build type uses -Os by default
# Additional size optimization flags:
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "-Wl,--gc-sections -Wl,--strip-all")
```

### Feature Toggle Impact on Binary Size

| Feature Toggled Off      | Binary Size Reduction |
|--------------------------|----------------------|
| `EB_ENABLE_JS=OFF`       | -48 KB to -64 KB     |
| `EB_ENABLE_TLS=OFF`      | -64 KB to -96 KB     |
| `EB_ENABLE_IMAGES=OFF`   | -16 KB to -24 KB     |
| `EB_ENABLE_TABLES=OFF`   | -8 KB to -12 KB      |
| `EB_ENABLE_FLEXBOX=OFF`  | -12 KB to -16 KB     |
| `EB_ENABLE_CACHE=OFF`    | -4 KB to -8 KB       |

## 12.3 Profiling

eBrowser includes built-in profiling instrumentation that can be enabled at
build time:

```cmake
# Enable profiling
>>> cmake -DEB_ENABLE_PROFILING=ON ..
```

### Profiling API

```c
/* Start a profiling section */
void eb_profile_begin(const char *section_name);

/* End a profiling section */
void eb_profile_end(const char *section_name);

/* Print profiling report */
void eb_profile_report(void);
```

### Example Profiling Output

```
eBrowser Profiling Report
=========================================================
Section              Calls    Total(ms)   Avg(ms)
---------------------------------------------------------
html_parse             1       12.4       12.400
css_parse              3        4.2        1.400
style_resolve          1        3.1        3.100
layout_compute         1        5.8        5.800
render_paint           1        2.3        2.300
network_fetch          1       85.6       85.600
---------------------------------------------------------
Total                          113.4
=========================================================
```

## 12.4 Rendering Performance

| Benchmark                    | ARM Cortex-M7 @400MHz | ARM Cortex-A53 @1.2GHz |
|------------------------------|----------------------|------------------------|
| Simple page (20 elements)    | 8 ms                 | 2 ms                   |
| Medium page (100 elements)   | 35 ms                | 8 ms                   |
| Complex page (500 elements)  | 180 ms               | 40 ms                  |
| Full repaint (800x480)       | 12 ms                | 3 ms                   |
| Incremental update (1 elem)  | 0.5 ms               | 0.1 ms                 |

---

# Chapter 13: Deployment on Embedded Targets

## 13.1 Deployment Overview

Deploying eBrowser on an embedded target involves four main steps:

1. **Cross-compile** eBrowser for the target architecture
2. **Integrate** the library into the target application
3. **Configure** the PAL for the target platform
4. **Flash** the firmware to the target device

## 13.2 Example: STM32F7 Discovery Board

### Hardware Specifications

| Parameter        | Value                              |
|------------------|------------------------------------|
| MCU              | STM32F746NG (ARM Cortex-M7)       |
| Clock            | 216 MHz                            |
| Flash            | 1 MB                               |
| RAM              | 340 KB (DTCM + SRAM1 + SRAM2)     |
| Display          | 4.3" TFT, 480x272, RGB565         |
| Touch            | Capacitive touch controller        |

### Build Configuration

```bash
>>> cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/stm32f7.cmake \
          -DCMAKE_BUILD_TYPE=MinSizeRel \
          -DEB_ENABLE_JS=OFF \
          -DEB_ENABLE_TLS=OFF \
          -DEB_DISPLAY_FRAMEBUFFER=ON \
          -DEB_DISPLAY_SDL2=OFF \
          -DEB_DOM_POOL_SIZE=256 \
          -DEB_CACHE_DEFAULT_SIZE=16384 ..
>>> make -j$(nproc)
```

### Integration Code

```c
#include "eBrowser/eBrowser.h"
#include "stm32f7xx_hal.h"

/* Framebuffer in external SDRAM */
static uint16_t framebuffer[480 * 272] __attribute__((section(".sdram")));

int main(void) {
    HAL_Init();
    SystemClock_Config();
    BSP_LCD_Init();
    BSP_TS_Init(480, 272);

    eb_config cfg = eb_config_default();
    cfg.width  = 480;
    cfg.height = 272;
    cfg.pixel_format = EB_PIXEL_RGB565;
    cfg.enable_js = false;
    cfg.enable_tls = false;
    cfg.cache_size = 16384;
    cfg.max_dom_nodes = 256;

    eb_instance *browser = eb_create(&cfg);
    eb_set_render_target(browser, framebuffer);
    eb_navigate(browser, "http://192.168.1.100/dashboard.html");

    while (1) {
        /* Poll touch input */
        TS_StateTypeDef ts_state;
        BSP_TS_GetState(&ts_state);
        if (ts_state.touchDetected) {
            eb_input_event ev = {
                .type = EB_EVENT_TOUCH_START,
                .touch = { .x = ts_state.touchX[0],
                           .y = ts_state.touchY[0],
                           .id = 0 }
            };
            eb_dispatch_event(browser, &ev);
        }

        eb_poll(browser, 16);
        if (eb_needs_repaint(browser)) {
            eb_repaint(browser);
            BSP_LCD_DrawBitmap(0, 0, (uint8_t *)framebuffer);
        }
    }
}
```

## 13.3 Example: ESP32-S3 with LVGL

### Hardware Specifications

| Parameter        | Value                              |
|------------------|------------------------------------|
| MCU              | ESP32-S3 (Xtensa LX7 dual-core)   |
| Clock            | 240 MHz                            |
| Flash            | 16 MB (external)                   |
| PSRAM            | 8 MB (external)                    |
| RAM              | 512 KB (internal)                  |
| Display          | 3.5" IPS, 480x320, SPI interface   |

### Build Configuration

```bash
>>> export IDF_PATH=/opt/esp-idf
>>> cmake -DCMAKE_TOOLCHAIN_FILE=$IDF_PATH/tools/cmake/toolchain-esp32s3.cmake \
          -DCMAKE_BUILD_TYPE=MinSizeRel \
          -DEB_DISPLAY_LVGL=ON \
          -DEB_ENABLE_TLS=ON \
          -DEB_ENABLE_JS=OFF \
          -DEB_DOM_POOL_SIZE=512 ..
>>> make -j$(nproc)
```

## 13.4 Example: Raspberry Pi (Embedded Linux)

### Build Configuration

```bash
# Native build on Raspberry Pi
>>> cmake -DCMAKE_BUILD_TYPE=Release \
          -DEB_DISPLAY_FRAMEBUFFER=ON \
          -DEB_ENABLE_JS=ON \
          -DEB_ENABLE_TLS=ON ..
>>> make -j4
>>> ./eBrowser --resolution 1024x600 http://localhost:8080
```

## 13.5 Deployment Checklist

| Step | Task                                    | Verified |
|------|-----------------------------------------|----------|
| 1    | Cross-compiler installed and tested     | [ ]      |
| 2    | Toolchain file created for target       | [ ]      |
| 3    | PAL implemented for target platform     | [ ]      |
| 4    | Platform test suite passes              | [ ]      |
| 5    | Build configuration optimized for size  | [ ]      |
| 6    | Memory budgets verified                 | [ ]      |
| 7    | Display output verified on target       | [ ]      |
| 8    | Input handling tested on target         | [ ]      |
| 9    | Network connectivity tested             | [ ]      |
| 10   | Performance benchmarks acceptable       | [ ]      |

## 13.6 Memory Budget Planning

When deploying to constrained targets, plan your memory budget carefully:

```
Total RAM Available:    340 KB (STM32F746)

System/OS overhead:     -40 KB
Stack:                  -16 KB
Application code:       -20 KB
Framebuffer (480x272):  -255 KB (use external SDRAM)
                        --------
Available for eBrowser: ~264 KB (with SDRAM framebuffer)

eBrowser allocation:
  DOM pool (256 nodes):  12 KB
  CSS styles:             6 KB
  Layout boxes:           4 KB
  String interning:       2 KB
  Network buffers:        8 KB
  Response cache:        16 KB
  Working memory:        20 KB
                        --------
  Total eBrowser:        68 KB

Remaining for app:     ~196 KB
```

---

# Appendix A: CLI Reference

## A.1 Command Syntax

```
eBrowser [options] <url>
```

## A.2 Options

| Flag / Option              | Argument       | Default   | Description                           |
|----------------------------|----------------|-----------|---------------------------------------|
| `--resolution WxH`         | `WIDTHxHEIGHT` | `800x480` | Set viewport resolution               |
| `--no-js`                  | (none)         | JS on     | Disable JavaScript engine             |
| `--cache-size MB`          | Integer (MB)   | `1`       | Set response cache size in megabytes  |
| `--log-level LEVEL`        | `debug`/`info`/`warn`/`error` | `warn` | Set logging verbosity |
| `--fullscreen`             | (none)         | windowed  | Launch in fullscreen mode             |
| `--pixel-format FORMAT`    | `rgb565`/`rgb888`/`argb8888` | `rgb565` | Set pixel format      |
| `--no-images`              | (none)         | images on | Disable image loading                 |
| `--no-cache`               | (none)         | cache on  | Disable response caching              |
| `--no-tls`                 | (none)         | TLS on    | Disable TLS (HTTP only)               |
| `--user-agent STRING`      | String         | default   | Set custom User-Agent header          |
| `--proxy HOST:PORT`        | `host:port`    | none      | Route traffic through HTTP proxy      |
| `--timeout SECONDS`        | Integer        | `30`      | Set network timeout in seconds        |
| `--dump-dom`               | (none)         | off       | Print DOM tree to stdout and exit     |
| `--dump-layout`            | (none)         | off       | Print layout tree to stdout and exit  |
| `--profile`                | (none)         | off       | Enable profiling output               |
| `--help`                   | (none)         | -         | Display help message and exit         |
| `--version`                | (none)         | -         | Display version information and exit  |

## A.3 Usage Examples

```bash
# Basic usage - open a URL
>>> eBrowser https://example.com

# Custom resolution with no JavaScript
>>> eBrowser --resolution 480x320 --no-js https://iot-dashboard.local

# Debug logging with profiling
>>> eBrowser --log-level debug --profile https://example.com

# Headless DOM dump for testing
>>> eBrowser --dump-dom https://example.com > dom_output.txt

# Fullscreen kiosk mode
>>> eBrowser --fullscreen --no-js --cache-size 4 https://kiosk.local/home

# Minimal memory configuration
>>> eBrowser --no-js --no-images --no-cache --resolution 320x240 \
    https://simple-page.local

# Using a proxy
>>> eBrowser --proxy 192.168.1.1:8080 --timeout 10 https://example.com
```

---

# Appendix B: Configuration Reference

## B.1 CMake Options - Complete Reference

### Core Features

| Option                    | Type | Default | Description                               |
|---------------------------|------|---------|-------------------------------------------|
| `EB_ENABLE_JS`            | BOOL | OFF     | Enable the JavaScript engine              |
| `EB_ENABLE_TLS`           | BOOL | ON      | Enable TLS/HTTPS support                  |
| `EB_ENABLE_IMAGES`        | BOOL | ON      | Enable image decoding and rendering       |
| `EB_ENABLE_TOUCH`         | BOOL | ON      | Enable touch input support                |
| `EB_ENABLE_KEYBOARD`      | BOOL | ON      | Enable keyboard input support             |
| `EB_ENABLE_TABLES`        | BOOL | ON      | Enable HTML table layout                  |
| `EB_ENABLE_FLEXBOX`       | BOOL | OFF     | Enable CSS flexbox layout                 |
| `EB_ENABLE_CACHE`         | BOOL | ON      | Enable HTTP response caching              |
| `EB_ENABLE_FORMS`         | BOOL | ON      | Enable HTML form element support          |
| `EB_ENABLE_PROFILING`     | BOOL | OFF     | Enable built-in profiling instrumentation |

### Display Backends

| Option                    | Type | Default | Description                               |
|---------------------------|------|---------|-------------------------------------------|
| `EB_DISPLAY_SDL2`         | BOOL | ON      | Build the SDL2 display backend            |
| `EB_DISPLAY_LVGL`         | BOOL | OFF     | Build the LVGL display backend            |
| `EB_DISPLAY_FRAMEBUFFER`  | BOOL | OFF     | Build the direct framebuffer backend      |

### Memory Configuration

| Option                    | Type | Default | Description                               |
|---------------------------|------|---------|-------------------------------------------|
| `EB_DOM_POOL_SIZE`        | INT  | 1024    | Maximum number of DOM nodes in the pool   |
| `EB_STYLE_POOL_SIZE`      | INT  | 512     | Maximum style objects in the pool         |
| `EB_LAYOUT_POOL_SIZE`     | INT  | 512     | Maximum layout box objects in the pool    |
| `EB_STRING_INTERN_SIZE`   | INT  | 256     | Size of the string interning table        |
| `EB_NET_BUFFER_SIZE`      | INT  | 4096    | Network receive buffer size (bytes)       |
| `EB_CACHE_DEFAULT_SIZE`   | INT  | 65536   | Default response cache size (bytes)       |
| `EB_MAX_CONNECTIONS`      | INT  | 4       | Maximum simultaneous HTTP connections     |
| `EB_MAX_REDIRECTS`        | INT  | 5       | Maximum HTTP redirect hops                |

### TLS Configuration

| Option                    | Type   | Default  | Description                             |
|---------------------------|--------|----------|-----------------------------------------|
| `EB_TLS_BACKEND`          | STRING | mbedtls  | TLS implementation (mbedtls, platform)  |
| `EB_TLS_CA_BUNDLE`        | PATH   | (none)   | Path to CA certificate bundle           |
| `EB_TLS_VERIFY`           | BOOL   | ON       | Enable certificate verification         |

### Build Configuration

| Option                    | Type | Default | Description                               |
|---------------------------|------|---------|-------------------------------------------|
| `EB_BUILD_TESTS`          | BOOL | OFF     | Build the test suite                      |
| `EB_BUILD_EXAMPLES`       | BOOL | OFF     | Build example applications                |
| `EB_BUILD_DOCS`           | BOOL | OFF     | Build documentation                       |
| `EB_BUILD_CLI`            | BOOL | ON      | Build the CLI executable                  |
| `EB_STATIC_LINK`          | BOOL | ON      | Build as static library                   |
| `EB_SHARED_LINK`          | BOOL | OFF     | Build as shared library                   |

---

# Appendix C: Troubleshooting Guide

## C.1 Build Issues

### Problem: CMake cannot find SDL2

```
-- Could NOT find SDL2 (missing: SDL2_LIBRARY SDL2_INCLUDE_DIR)
```

**Solution:** Install SDL2 development libraries:

```bash
# Ubuntu/Debian
>>> sudo apt-get install libsdl2-dev

# macOS
>>> brew install sdl2

# Or disable SDL2 if not needed
>>> cmake -DEB_DISPLAY_SDL2=OFF ..
```

### Problem: Compiler version too old

```
error: unrecognized command line option '-std=c11'
```

**Solution:** Upgrade your compiler to GCC >= 9 or Clang >= 11:

```bash
>>> sudo apt-get install gcc-12 g++-12
>>> cmake -DCMAKE_C_COMPILER=gcc-12 -DCMAKE_CXX_COMPILER=g++-12 ..
```

### Problem: Cross-compilation linker errors

```
undefined reference to `eb_pal_alloc`
```

**Solution:** Ensure your PAL implementation file is included in the build.
Check your toolchain file and verify the platform source is linked:

```cmake
# In your CMakeLists.txt or toolchain file
target_sources(eBrowser PRIVATE platform/pal_<yourplatform>.c)
```

## C.2 Runtime Issues

### Problem: Out of DOM nodes

```
EB_ERR_NOMEM: DOM node pool exhausted (1024/1024 nodes used)
```

**Solution:** Increase the DOM pool size:

```bash
>>> cmake -DEB_DOM_POOL_SIZE=2048 ..
```

Or simplify the HTML content to use fewer elements.

### Problem: Blank screen after navigation

**Possible causes:**

1. **Network error** - Check that the URL is reachable from the device
2. **Display not initialized** - Verify PAL display functions are implemented
3. **Framebuffer not flushed** - Ensure `eb_pal_display_flush()` is called
4. **Wrong pixel format** - Verify the pixel format matches the display hardware

**Diagnostic steps:**

```bash
# Enable debug logging
>>> eBrowser --log-level debug https://example.com

# Dump the DOM to verify parsing works
>>> eBrowser --dump-dom https://example.com
```

### Problem: Touch input not working

**Possible causes:**

1. **Touch driver not initialized** - Check BSP touch initialization
2. **Coordinate mapping incorrect** - Verify touch coordinates match display
3. **Event dispatch missing** - Ensure `eb_dispatch_event()` is called
4. **Touch feature disabled** - Rebuild with `EB_ENABLE_TOUCH=ON`

### Problem: TLS handshake failure

```
EB_ERR_TLS: TLS handshake failed: certificate verification error
```

**Solution:**

```bash
# Provide a CA certificate bundle
>>> cmake -DEB_TLS_CA_BUNDLE=/path/to/ca-certificates.crt ..

# Or disable certificate verification (development only!)
>>> cmake -DEB_TLS_VERIFY=OFF ..
```

### Problem: High memory usage

**Diagnostic steps:**

1. Enable profiling: `cmake -DEB_ENABLE_PROFILING=ON`
2. Run with profiling: `eBrowser --profile <url>`
3. Review the memory usage report
4. Reduce pool sizes and cache size as needed
5. Disable unused features (JS, images, tables)

## C.3 Performance Issues

### Problem: Slow page rendering

**Optimization strategies:**

1. Use `MinSizeRel` or `Release` build type
2. Reduce DOM complexity in the HTML content
3. Minimize CSS selector complexity (avoid deep nesting)
4. Disable unused features to reduce code path overhead
5. Increase network buffer size if network is the bottleneck

### Problem: Flickering display

**Solution:** Implement double buffering:

```c
/* Allocate two framebuffers */
static uint16_t fb_front[WIDTH * HEIGHT];
static uint16_t fb_back[WIDTH * HEIGHT];

/* Render to back buffer */
eb_set_render_target(browser, fb_back);
eb_repaint(browser);

/* Swap buffers atomically */
memcpy(fb_front, fb_back, sizeof(fb_front));
eb_pal_display_flush(0, 0, WIDTH, HEIGHT);
```

---

# Glossary of Terms

| Term                        | Definition                                                  |
|-----------------------------|-------------------------------------------------------------|
| **ASan**                    | Address Sanitizer - a memory error detector                 |
| **Bare-metal**              | Running directly on hardware without an operating system    |
| **Box model**               | CSS model defining margin, border, padding, and content     |
| **Cascade**                 | The CSS algorithm for resolving conflicting style rules     |
| **CMake**                   | Cross-platform build system generator used by eBrowser      |
| **Cross-compilation**       | Compiling code on one platform to run on a different one    |
| **DOM**                     | Document Object Model - tree representation of HTML         |
| **EmbeddedOS (EOS)**        | The embedded operating system ecosystem eBrowser belongs to |
| **Framebuffer**             | A memory region holding pixel data for display output       |
| **HMI**                     | Human-Machine Interface - visual control panels             |
| **HTML5**                   | The fifth version of the HyperText Markup Language          |
| **HTTP/1.1**                | Hypertext Transfer Protocol version 1.1                     |
| **IoT**                     | Internet of Things - network-connected embedded devices     |
| **LVGL**                    | Light and Versatile Graphics Library for embedded GUIs      |
| **mbedTLS**                 | Lightweight TLS library for embedded systems                |
| **MCU**                     | Microcontroller Unit - a compact integrated circuit         |
| **MSan**                    | Memory Sanitizer - detects uninitialized memory reads       |
| **PAL**                     | Platform Abstraction Layer - portability interface          |
| **Pool allocation**         | Pre-allocating a fixed number of objects for reuse          |
| **RGB565**                  | 16-bit pixel format: 5 red, 6 green, 5 blue bits           |
| **RGB888**                  | 24-bit pixel format: 8 bits per red, green, blue channel    |
| **RTOS**                    | Real-Time Operating System                                  |
| **SDL2**                    | Simple DirectMedia Layer - multimedia library               |
| **Specificity**             | CSS rule for determining which styles take precedence       |
| **String interning**        | Storing one copy of each distinct string value              |
| **TLS**                     | Transport Layer Security - encryption protocol              |
| **Toolchain file**          | CMake file specifying cross-compilation settings            |
| **Tokenizer**               | Parser component that breaks input into discrete tokens     |
| **Viewport**                | The visible display area for rendering web content          |
| **Void element**            | HTML element that cannot have children (e.g., `<br>`)       |

---

## Colophon

This reference was produced as part of the EmbeddedOS documentation project.

**eBrowser - Lightweight Embedded Web Browser: Product Reference**
First Edition, April 2026

Copyright (c) 2026 Srikanth Patchava & EmbeddedOS Contributors.
Licensed under the MIT License.

For the latest version of this document and the eBrowser source code, visit
the EmbeddedOS project repository.

---

*End of Document*

## References

::: {#refs}
:::
