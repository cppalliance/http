Looking at the source code, I can reverse engineer the ABNF grammar for path-to-regexp. Let me analyze the tokenizer and parser logic:

## ABNF Grammar for path-to-regexp

```abnf
; Top-level path pattern
path            = *token

; Token types
token           = text / param / wildcard / group

; Text is one or more literal characters (possibly escaped)
text            = 1*(char / escaped-char)

; Named parameter: :name or :"quoted"
param           = ":" name

; Wildcard parameter: *name or *"quoted"
wildcard        = "*" name

; Optional group (all-or-nothing)
group           = "{" *token "}"

; Parameter name (two forms)
name            = identifier / quoted-name

; JavaScript-style identifier
identifier      = id-start *id-continue
id-start        = "$" / "_" / ID_Start
                  ; ID_Start = Unicode category ID_Start
id-continue     = "$" / %x200C / %x200D / ID_Continue
                  ; %x200C = ZWNJ, %x200D = ZWJ
                  ; ID_Continue = Unicode category ID_Continue

; Quoted parameter name (allows any characters)
quoted-name     = DQUOTE *quoted-char DQUOTE
quoted-char     = escaped-char / %x00-21 / %x23-5B / %x5D-10FFFF
                  ; any char except DQUOTE and backslash, or escaped

; Escape sequence
escaped-char    = "\" CHAR
                  ; backslash followed by any character

; Regular character (not special)
char            = %x00-20 / %x23-27 / %x2C / %x2D / "." / %x30-39 
               /  %x3B-3F / %x41-5A / %x5E-7A / %x7C / %x7E-10FFFF
                  ; excludes: { } ( ) [ ] + ? ! : * \

; Characters requiring escape in text
special         = "{" / "}" / "(" / ")" / "[" / "]" 
               /  "+" / "?" / "!" / ":" / "*" / "\"

; Reserved (parsed but invalid in current version)
reserved        = "(" / ")" / "[" / "]" / "+" / "?" / "!"
```

## Summary Table

| Syntax | Meaning |
|--------|---------|
| `:name` | Named parameter (matches non-delimiter chars) |
| `*name` | Wildcard parameter (matches everything including `/`) |
| `{...}` | Optional group (matches all-or-nothing) |
| `\"...\"` | Quoted name (allows special chars in param name) |
| `\X` | Escape character X (literal) |
| `(`, `)`, `[`, `]`, `+`, `?`, `!` | Reserved (tokenized but cause parse error) |

## Examples

```
/users/:id          ; literal "/users/" + param "id"
/files/*path        ; literal "/files/" + wildcard "path"
/api{/v:version}    ; "/api" + optional group "/v" + param "version"
/:foo-:bar          ; param "foo" + literal "-" + param "bar"  
/:"with spaces"     ; param with quoted name containing spaces
/path\:literal      ; escaped colon (literal text)
```

## Key Observations from Code

1. **No regex modifiers** - Unlike older versions, `(pattern)` after params is reserved but not implemented
2. **Groups are optional** - `{...}` means "include all or none" for path generation
3. **Backtracking protection** - Consecutive params require separator text between them
4. **Unicode-aware** - Parameter names support full Unicode identifiers