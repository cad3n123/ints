# Ints

Ints is a simple, array-oriented programming language built from scratch in C++, designed around a single data type: arrays of integers.

It supports:

- Fixed and growable arrays
- User-defined functions with strict input/output arity
- Arithmetic, slicing, method chaining, and conditionals
- A minimal and expressive syntax for manipulating integer sequences

> This was my first real project in C++, which I rarely use. It’s not elegant, but it works, and honestly, that’s all I wanted.

---

## Getting Started

### Build Instructions (Linux / Windows w/ Make)

```bash
make
```

### Run Your Program

```bash
./main.exe   # on Windows
./main       # on Linux/Mac
```

The program looks for a file called test.ints in the root directory.

## Language Example

```ints
fn double(x: [+]) -> [+] {
    return x + x;
}

fn main() -> [+] {
    let nums: [3] = [1, 2, 3];
    let doubled: [3] = double(nums);
    print(doubled + "000");
}

```
