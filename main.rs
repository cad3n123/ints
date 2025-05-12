use "ints/runtime/interpreter.ints"

fn main(argc: [1], argv: [+]) -> [1] {
    if argc < [1] {
        print("Usage: <filename> [args...]\n");
        return [1];
    }
    let fileName: [+] = getFileName(argv);
    let args: [+] = argv[argv[0] + [1]:];
}

fn getFileName(argv: [2+]) -> [+] {
    let fileNameLength: [1] = argv[0];
    return argv[1:fileNameLength + [1]];
}
