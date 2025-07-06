# Tile Properties

The properties will contain general behavior of specified domains.

---

## Use Cases

**entity_properties.yaml**: Will contain base default stats of every type of NPC and the player.

**tile_properties.yaml**: Will contain how a tile shall behave, is it breakable, intractable etc.

---

---

## How to write properties

### Structure of the documents

The document structure will be pretty simple and straight forward.

```yaml
element1:
  property1: val1
  property2: [val2.0, val2.1, val2.3, ...]
  ...

element2:
  property1: val1
  property2: [val2.0, val2.1, val2.3, ...]
  ...
...
```

You can see in the above example no nesting was done for particular properties. This standard shall be upheld with all the properties of a particular element being flat with no internal nesting whatsoever.
Technical Details: You can nest as much as you want, but only the key-values pair at the end of the nesting will be given. But doing nesting will mess up the ID system of the parser. So that's why you should avoid it.

Elements of a particular group may not be nested in a top level tag.
The following example is how **NOT** to structure your elements.
```yaml
  element_type1:
    element1:
      property1: val1
      property2: [val2.0, val2.1, val2.3, ...]
      ...

    element2:
      property1: val1
      property2: [val2.0, val2.1, val2.3, ...]
      ...
    ...

  element_type2:
    element1:
      property1: val1
      property2: [val2.0, val2.1, val2.3, ...]
      ...

    element2:
      property1: val1
      property2: [val2.0, val2.1, val2.3, ...]
      ...
    ...
```

The correct way to do it is to organize different types of elements into different file separate from each other

```txt
properties/-|
            |
            |- element_type1.yaml
            |- element_type2.yaml
            |...
```

---

---

### Formatting

- In **sequences**, prefer the '[]' definition over the '-' definition.

The following definition is invalid:

```yaml
  element1:
    property1:
      - val1
      - val2
      - val3
```

Instead prefer:

```yaml
  element1:
    property1: [val1, val2, val3]
```

- All scalars should be in lowercase.

---
