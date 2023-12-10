## JSONParser Module

### Overview

The Moddable SDK JSON Parser is designed for parsing JSON data streams in embedded systems. It supports character-by-character extraction in C, enabling efficient processing of large files in low-memory environments. JavaScript interfaces offer flexibility for diverse parsing needs, such as using matching functions to extract essential data or constructing a complete JSON tree from the input stream.

### License and Credits

The JSON Parser modules and examples are released under the [MIT License](https://opensource.org/license/mit/). Please refer to the file named [COPYING](COPYING) in the source for details. They are contributed to Moddable under the [Contributor License Agreement](../../licenses/readme.md#contributor-license-agreement).

#### Ragel

The JSON Parser module utilizes Ragel to generate robust [parser code](./jsonparser.c). Ragel was designed and developed by [Adrian Thurston](https://github.com/adrian-thurston), and it is known for its efficiency in generating state machine-based code for lexical analyzers and parsers. For more information about using Ragel, refer to the [Ragel State Machine Compiler](http://www.colm.net/open-source/ragel/) documentation.

#### HeliMods

Special thanks to [HeliMods](https://www.helimods.com) and [Marc Treble](https://github.com/mtreble) for initiating and supporting the Meeting Room Display project. It turned out that the project required parsing JSON data streams greater than the available memory on a [Moddable Three](https://www.moddable.com/moddable-three) device.

### Classes

#### JSONParser

Constructor
```javascript
constructor(vpt) @ "xs_jsonparser_constructor";
```
Creates a new `JSONParser` instance.

Methods
- `initialize(keys, initialStackDepth, initialBufferSize) @ "xs_jsonparser_initialize";`
  - Initializes the JSON parser with the specified keys, stack depth, and buffer size.
- `receive(segment, start, end) @ "xs_jsonparser_receive";`
  - Receives a JSON data segment for parsing.
- `terminate() @ "xs_jsonparser_terminate";`
  - Terminates the JSON parsing process.

Static Properties
- `JSONParser.failure: -1`
  - Indicates a parsing failure.
- `JSONParser.receive: 0`
  - Indicates successful reception of JSON data.
- `JSONParser.success: 1`
  - Indicates successful completion of JSON parsing.

#### Matcher

Constructor
```javascript
constructor(func = undefined, init = undefined, term = undefined);
```
Creates a new `Matcher` instance with optional initialization, matching, and termination functions.

Methods
- `initialize(vpt);`
  - Initializes the Matcher.
- `match(vpt, node);`
  - Matches the current node using the provided function.
- `terminate(vpt);`
  - Terminates the matching process.

#### Node

Constructor
```javascript
constructor(type, prev = undefined);
```
Creates a new `Node` instance with a specified type and optional reference to the previous node.

Properties
- `data`
  - Returns the data associated with the node.

Methods
- `up(count = 1, nodeType = undefined, nodeText = undefined);`
  - Moves up the node hierarchy by a specified count, filtering by type and text if provided.

#### VPT

Constructor
```javascript
constructor(matcher = undefined);
```
Creates a new `VPT` (Virtual Parse Tree) instance with optional matcher.

Methods
- `initialize();`
  - Initializes the VPT and its associated matcher.
- `makeNode(nodeType, prev = undefined);`
  - Creates a new node of the specified type with an optional reference to the previous node.
- `pop(nodeType);`
  - Pops the current node if its type matches the specified type.
- `push(nodeType);`
  - Pushes a new node of the specified type onto the parse tree.
- `setText(text);`
  - Sets the text of the current node.
- `terminate();`
  - Terminates the parsing process.

### Constants

#### NodeType

An enumeration of node types:
- `null: 0`
- `false: 1`
- `true: 2`
- `number: 3`
- `string: 4`
- `array: 5`
- `object: 6`
- `field: 7`
- `root: 8`

### Usage

1. Import the necessary classes: `JSONParser`, `Matcher`, `Node`, `NodeType`, and `VPT`. 
2. Create instances of `JSONParser` and `VPT` to initiate the parsing process. 
3. Customize the parsing behavior by implementing a `Matcher` with appropriate functions. 
4. Use the provided methods to manipulate the parse tree and extract information.

### Code Generation

Confirm Ragel version 6.10 (Stable):
```bash
ragel --version
```

Example output:
```text
Ragel State Machine Compiler version 6.10 March 2017
Copyright (c) 2001-2009 by Adrian Thurston
```

Generate `-T1` output style code:
```bash
ragel -T1 jsonparser.rl > jsonparser.c
```

Update `jsonparser.c` by adding `ICACHE_XS6RO_ATTR` and integrating read macros for accessing static const data in ROM. Using the regular expression search and replace feature of your IDE:

Step 1
- Search: `\[\] = \{`
- Replace: `[] ICACHE_XS6RO_ATTR = {`

Step 2
- Search: `(?:\*(_mid) )`
- Replace: `c_read8( $1 )`

Step 3
- Search: `(_JSON_(?:index|key)_offsets\[[^\]]+\])`
- Replace: `(unsigned char)c_read8( &$1 )`

Step 4
- Search: `((_JSON_(?:indicies|(?:range|single)_lengths|trans_(?:actions|targs))|_mid)\[[^\]]+\])`
- Replace: `c_read8( &$1 )`

Compile the code and test the changes to confirm that everything works as expected.

### Limitations and Known Issues

- The CESU-8 compatibility encoding scheme is not supported.
- `\u` hex code value substitution is not yet implemented (value becomes ?).
- JSON field names consist of plain text, and special characters are not unescaped.
- ~~Needs ICACHE and read macros to store and access static const data in ROM.~~
- There are no [jsonparser.d.ts](../../typings/jsonparser.d.ts) type definitions and no [jsonparser](../../tests/contributed/jsonparser) tests.
- There are no [jsontree.d.ts](../../typings/jsontree.d.ts) type definitions and no [jsontree](../../tests/contributed/jsonparser/jsontree) tests.
- JSON.parse() like reviver functionality is not supported.
- xsUnknownError with and without xsTry/xsCatch.
- Some `TODO: review` items to address.

### References

- https://ecma-international.org/publications-and-standards/standards/ecma-404
- https://ecma-international.org/wp-content/uploads/ECMA-404_2nd_edition_december_2017.pdf
- https://www.colm.net/files/ragel/ragel-guide-6.10.pdf

<!--
Additional References
https://419.ecma-international.org
https://daniel.haxx.se/blog/2016/11/14/i-have-toyota-corola/
https://ecma-international.org/publications-and-standards/standards/ecma-419/
https://ecma-international.org/wp-content/uploads/ECMA-419_2nd_edition_june_2023.pdf
https://unicode.org/charts/PDF/UFFF0.pdf
https://unicode.org/charts/nameslist/n_FFF0.html
https://www.moddable.com/embedded-javascript
-->

<!--
Creating .d.ts Files
Search: `@ "xs_jsonparser_(constructor|initialize|receive|terminate)"`
Replace: `{}`
Search: `@ "xs_jsonparser_destructor" `
Replace: `{}`
https://github.com/nvm-sh/nvm
https://github.com/tsdjs/tsd (not used, but could be interesting)
https://www.typescriptlang.org/docs/handbook/declaration-files/dts-from-js.html
nvm install 16
mkdir types
npx -p typescript tsc jsonparser.js --declaration --allowJs --emitDeclarationOnly --outDir types
npx -p typescript tsc jsontree/jsontree.js --declaration --allowJs --emitDeclarationOnly --outDir types
-->

<!--
import() and Tree shaking
https://developer.mozilla.org/en-US/docs/Glossary/Tree_shaking
https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/import
https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/import
https://tc39.es/ecma262/multipage/ecmascript-language-expressions.html#sec-import-calls
-->

<!--
General questions, relating to UTF or Encoding Form
https://unicode.org/faq/utf_bom.html
CESU-8	https://unicode.org/faq/utf_bom.html#utf8-4
UCS-2	https://unicode.org/faq/utf_bom.html#utf16-11
-->
