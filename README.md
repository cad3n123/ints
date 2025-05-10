# Ints

Ints is a simple, array-oriented programming language built from scratch in C++, designed around a single data type: arrays of integers.

It supports:

* Fixed and growable arrays
* User-defined functions with strict input/output arity
* Arithmetic, slicing, method chaining, and conditionals
* A minimal and expressive syntax for manipulating integer sequences

> This was my first real project in C++, which I rarely use. It’s not elegant, but it works, and honestly, that’s all I wanted.

---

## Getting Started

### Build Instructions (Linux / Windows w/ Make)

```bash
make
```

### Run Your Program

```bash
./main.exe <file.ints> [arg1 arg2 ...]   # on Windows
./main <file.ints> [arg1 arg2 ...]       # on Linux/Mac
```

---

## Passing Arguments to the Program

If you pass extra arguments on the command line, they are passed to the `main` function as a single array.

Each string argument is encoded as an array of integers where:

* The first value is the number of characters in the argument
* Followed by one integer per character (ASCII value)

These arguments are **flattened** into one array.

#### Example:

If you call:

```bash
./main example.ints hello 123
```

Then the `main` function in `example.ints` receives:

```ints
[5, 'h', 'e', 'l', 'l', 'o', 3, '1', '2', '3']
```

Internally, this becomes:

```ints
fn main(args: [+]) -> [+] {
    // args == [104, 101, 108, 108, 111, 3, 49, 50, 51]
    ...
}
```

Use slicing and method calls to extract individual arguments.

---

## Language Example

```ints
fn double(x: [+]) -> [+] {
    return x + x;
}

fn main(args: [+]) -> [+] {
    let str: [+] = args[1:6];
    print(str);
    return double([1, 2, 3]);
}
```

---

## Notes

* No strings, booleans, or floats—just arrays of integers
* Only top-level functions and array expressions
* Method chaining (`.append`, `.sqrt`) works directly on arrays

---

## License

MIT.
