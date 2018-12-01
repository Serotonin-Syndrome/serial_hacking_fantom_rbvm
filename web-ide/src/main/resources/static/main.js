const CLIENT_COMPATIBILITY = "0.1.12"

const LAST_COMPATIBILITY = localStorage.getItem("client_compatibility")
if (CLIENT_COMPATIBILITY != LAST_COMPATIBILITY) {
    localStorage.clear();
    localStorage.setItem("client_compatibility", CLIENT_COMPATIBILITY)
}


var editor = ace.edit("editor");
editor.setTheme("ace/theme/eclipse");
editor.session.setMode("ace/mode/c_cpp");


highlight_formats = {
    'cpp': "ace/mode/c_cpp",
    'c': "ace/mode/c_cpp"
};

selectedFile = null;

const InterfaceConsole = (function () {
    let _console = $('#console');
    let _data = [];
    let _lastFile = null;

    function _textify(text, jqElement) {
        jqElement.text(text);
        let html = jqElement.html();
        jqElement.html(html.replace(/\r?\n/g, '<br/>'));
        return jqElement;
    }

    function _displayOutput() {
        let data = _data;
        _console.children().remove();

        if (data.length === 0) {
            _console.addClass('unfilled');
        } else {
            _console.removeClass('unfilled');
            data.forEach(function (execData) {
                let title = execData.title;
                let stdout = execData.stdout;
                let stderr = execData.stderr;

                if (title && (stdout || stderr)) {
                    _console.append($('<h4 />').html(title));
                }
                if (stdout) {
                    _console.append(_textify(stdout, $('<div class="console-stdout" />')));
                }
                if (stderr) {
                    _console.append(_textify(stderr, $('<div class="console-stderr" />')));
                }
            });
        }
    }

    let exportInterface = {
        appendData: function (data) {
            if (!data)
                return;
            if (!data.forEach)
                data = [data];

            if (_lastFile != selectedFile) {
                _lastFile = selectedFile;
                _data = [];
            }

            for (let i = 0; i < data.length; i++)
                _data.push(data[i]);

            _displayOutput();

            return this;
        },
        setData: function (data) {
            return this.clear().appendData(data);
        },
        clear: function () {
            _lastFile = null;
            _data = [];
            _displayOutput();
            return this;
        }
    };

    exportInterface.clear();
    return exportInterface;
})();

class File {
    constructor(name) {
        this._data = {
            'name': name
        };
        this.load();
    }

    static getFileNames() {
        var preFiles = localStorage.getItem("filestorage");
        if (preFiles)
            return JSON.parse(preFiles);
        return [];
    }

    static getFiles() {
        return File.getFileNames().map(function (name) {
            return new File(name);
        });
    }

    // next using this object will throw NPE
    delete() {
        localStorage.removeItem("filestorage:" + this._data.name);
        var files = File.getFileNames();
        var index = files.indexOf(this.name);
        if (index >= 0)
            files.splice(index, 1);
        localStorage.setItem("filestorage", JSON.stringify(files));
        this._data = null;
    }

    save() {
        localStorage.setItem("filestorage:" + this._data.name, JSON.stringify(this._data));
        var files = File.getFileNames();
        if (files.indexOf(this.name) < 0)
            files.push(this.name);
        localStorage.setItem("filestorage", JSON.stringify(files));
    }

    load() {
        var preData = localStorage.getItem("filestorage:" + this._data.name);

        if (preData) {
            this._data = JSON.parse(preData);
        } else {
            this._data = {
                'name': this._data.name,
                'code': "",
                'compiled': null
            };
            this.save()
        }
    }

    get name() {
        return this._data.name;
    }

    get code() {
        this.load();
        return this._data.code;
    }

    set code(value) {
        this._data.code = value;
        this.save();
    }

    get maintainId() {
        this.load();
        return this._data.maintainId;
    }

    set maintainId(value) {
        this._data.maintainId = value;
        this.save();
    }

    get isSmartContract() {
        return $.trim(this.code).startsWith("//!SMART CONTRACT");
    }

    get compiled() {
        this.load();
        return this._data.compiled;
    }

    set compiled(value) {
        this._data.compiled = value;
        this.save();
    }

    get format() {
        let index = this.name.lastIndexOf('.');
        if (index < 0)
            return null;
        return this.name.substring(index + 1).toLowerCase();
    }
}


start_files_data = {
    "helloworld.c": {
        "code": "/*\n    This code shows static string and native functions work correctly.\n*/\n\n#include <stdio.h>\n\nint main()\n{\n    puts(\"Hello, world!\");\n}\n"
    },
    "set-global.c": {
        "code": "/*\n    This example shows setting global variable works correctly.\n*/\n\n#include <stdio.h>\n\nint x = 1;\n\nint main()\n{\n    printf(\"%d\\n\", x);\n    ++x;\n    printf(\"%d\\n\", x);\n}\n"
    },
    "eratosthenes_sieve.c": {
        "code": "/*\n    Eratosthenes sieve -- method of finding the prime numbers.\n    \n    This example shows loops and conditions work correctly.\n*/\n\n\n#include <stdio.h>\n#include <stdlib.h>\ntypedef unsigned long UL;\n\nint main() {\n#ifdef JUDGE\n    UL n = 10000;\n#else\n    UL n;\n    scanf(\"%lu\", &n);\n#endif\n\n    char *p = malloc(n + 1);\n    if (!p) {\n        puts(\"Out of memory.\");\n        return 1;\n    }\n    p[0] = p[1] = 0;\n    for (UL i = 2; i <= n; ++i) {\n        p[i] = 1;\n    }\n    for (UL i = 2; i <= n; ++i) {\n        if (p[i]) {\n            if (i * i <= n) {\n                for (UL j = i * i; j <= n; j += i) {\n                    p[j] = 0;\n                }\n            }\n            printf(\"%lu\\n\", i);\n        }\n    }\n}\n"
    },
    "brainfuck.cpp": {
        "code": "/*\n    A Brainfuck interpreter.\n\n    More complex example showing loops, conditions and switches work correctly.\n*/\n\n\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n\nstatic\nvoid *\nxmalloc(size_t n)\n{\n    void *p = malloc(n);\n    if (n && !p) {\n        puts(\"Out of memory.\");\n        exit(1);\n    }\n    return p;\n}\n\nstatic\nconst char *\njump(const char *p, bool forward)\n{\n    int balance = 0;\n    do {\n        switch (*p) {\n        case '[': ++balance; break;\n        case ']': --balance; break;\n        }\n        if (forward) {\n            ++p;\n        } else {\n            --p;\n        }\n    } while (balance != 0);\n    return p;\n}\n\nint main() {\n#ifdef JUDGE\n    // prints out \"Hello World!\"\n    const char *prog = \"++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.\";\n#else\n    char *buf = (char *) xmalloc(1024);\n    scanf(\"%1023s\", buf);\n    const char *prog = buf;\n#endif\n\n    const size_t NMEM = 30000;\n    char *p = (char *) xmalloc(NMEM);\n    for (size_t i = 0; i < NMEM; ++i) {\n        p[i] = 0;\n    }\n\n    for (; *prog; ++prog) {\n        switch (*prog) {\n            case '>':\n                ++p;\n                break;\n            case '<':\n                --p;\n                break;\n            case '+':\n                ++*p;\n                break;\n            case '-':\n                --*p;\n                break;\n            case '.':\n                printf(\"%c\", *p);\n                break;\n            case ',':\n                scanf(\"%c\", p);\n                break;\n            case '[':\n                if (!*p) {\n                    prog = jump(prog, true);\n                    --prog;\n                }\n                break;\n                break;\n            case ']':\n                prog = jump(prog, false);\n                break;\n        }\n    }\n}\n"
    },
    "recursion.c": {
        "code": "/*\n    Fibonacci number caclulator.\n\n    This is a simple example of recursion.\n*/\n\n\n#include <stdio.h>\n\nint fib(int n)\n{\n    return n < 2 ? 1 : fib(n - 1) + fib(n - 2);\n}\n\nint main()\n{\n    printf(\"%d\\n\", fib(10));\n}\n"
    },
    "recursive_descent.cpp": {
        "code": "/*\n    Recursive analysis of math expressions -- recursively evaluates the value of given expression.\n\n    More complex example of recursion.\n*/\n\n\n#include <assert.h>\n#include <stdio.h>\n\n\nint p = 0;\nextern const char s[];\n\nint getE();\n\n\nint getN() {\n    int val = 0;\n\n    int p0 = p;\n    while ('0' <= s[p] && s[p] <= '9') {\n        val = val * 10 + (s[p] - '0');   \n        ++p;\n    }\n\n    assert(p0 != p);\n\n    return val;\n}\n\nint getP() {\n    int val = 0;\n\n    if (s[p] == '(') {\n        ++p;\n        val = getE();\n\n        assert(s[p] == ')');\n        ++p;\n        return val;\n    }\n\n    val = getN();\n\n    return val;\n}\n\nint getT() {\n    int val = getP();\n\n    while (s[p] == '*' || s[p] == '/') {\n        char op = s[p];\n        ++p;\n        int val2 = getP();\n\n        if (op == '*')\n            val *= val2;\n        else\n            val /= val2;\n    }\n\n    return val;\n}\n\nint getE() {\n    int val = getT();\n\n    while (s[p] == '-' || s[p] == '+') {\n        char op = s[p];\n        ++p;\n        int val2 = getT();\n\n        if (op == '-')\n            val -= val2;\n        else\n            val += val2;\n    }\n\n    return val;\n}\n\nint getG() {\n    p = 0;\n\n    int val = getE();\n    \n    assert(s[p] == '\0');\n    ++p;\n\n    return val;\n}\n\n\n// math expression to evaluate\nconst char s[] = \"(2+3)*7-9/(2+1)\";\n\n\nint main() {\n    printf(\"%d\\n\", getG());\n}\n"
    },
    "struct.c": {
        "code": "/*\n    This example shows changing structures' fields and passing them to fucnctions work correctly. \n*/\n\n\n#include <stdio.h>\n\nstruct S {\n    int a;\n    int b;\n};\n\nvoid printf_data(struct S* s) {\n    printf(\"s = {%d, %d}\\n\", s->a, s->b);\n}\n\nint main()\n{\n    struct S s;\n    s.a = 5;\n    s.b = 7;\n    printf_data(&s);\n}\n"
    },
    "snake.cpp": {
        "code": "/*\n    This example shows classes and inheritance work correctly.\n*/\n\n\n#include <stdio.h>\n\n\nstruct Animal\n{\n    virtual void voice() = 0;\n};\n\n\nstruct Snake : public Animal\n{\n    virtual void voice() override\n    {\n        puts(\"HSSSssssssssss......\");\n    }\n};\n\n\nint\nmain()\n{\n    Snake Drake;\n    Drake.voice();\n}\n"
    },
    "bubble_sort.cpp": {
        "code": "/*\n    Bubble sort.\n\n    This example shows arrays, functions and templates work correctly.\n*/\n\n\n#include <stdio.h>\n#include <stdlib.h>\n#include <functional>\n#include <iterator>\n\n\ntemplate <class RandomAccessIterator,\n          class Comparator = std::less<typename std::iterator_traits<RandomAccessIterator>::value_type>>\nvoid bubble_sort(RandomAccessIterator first, \n                 RandomAccessIterator last, Comparator cmp = Comparator()) {\n    for (auto i = first; i != last; ++i)\n        for (auto j = i + 1; j != last; ++j) \n            if (cmp(*j, *i))\n                std::swap(*i, *j);\n}\n\n\nint main() {\n#ifdef JUDGE\n    size_t n = 7;\n    int *array = (int *) malloc(n * sizeof(int));\n    {\n        int *p = array;\n        *p++ = 1;\n        *p++ = 7;\n        *p++ = 2;\n        *p++ = 6;\n        *p++ = 4;\n        *p++ = 5;\n        *p++ = 3;\n    }\n#else\n    size_t n = 0;\n    scanf(\"%zu\", &n);\n    int* array = (int*)malloc(n * sizeof(int));\n    for (size_t i = 0; i < n; ++i)\n        scanf(\"%d\", &array[i]);\n#endif\n\n    bubble_sort(array, array + n);\n\n    for (size_t i = 0; i < n; ++i)\n        printf(\"%d \\n\", array[i]);\n\n    free(array);\n}\n"
    },
    "contract.cpp": {
        "code": "//!SMART CONTRACT\n\n/*\n    This is an example of a smart contract.\n    We also provide a UI on our web site to deploy it.\n    Creator's address is 123.\n\n    Moreover it contains an implementation of a hash table.\n\n\n    This complex example embodies many features of programming languages.\n*/\n\n#include <stdio.h>\n#include <stdint.h>\n#include <stdlib.h>\n#include <new>\n#include <utility>\n\n#define smart_contract\n\ntemplate<class T> static\nT *\nalloc_for(size_t n)\n{\n    void *p = malloc(sizeof(T) * n);\n    if (n && !p) {\n        puts(\"Out of memory.\");\n        exit(1);\n    }\n    return static_cast<T *>(p);\n}\n\ntemplate<class T, class ...Args>\nT *\ncreate(Args&& ...args)\n{\n    T *ptr = alloc_for<T>(1);\n    new (ptr) T(std::forward<Args>(args)...);\n    return ptr;\n}\n\nclass Hashtable\n{\n    static uintptr_t zalloc8_(size_t n)\n    {\n        uint64_t *p = alloc_for<uint64_t>(n);\n        for (size_t i = 0; i < n; ++i) {\n            p[i] = 0;\n        }\n        return reinterpret_cast<uintptr_t>(p);\n    }\n\n    uint64_t ndata_;\n    uintptr_t keys_;\n    uintptr_t values_;\n\n    uint64_t *\n    get_keys_ptr_() const\n    {\n        return reinterpret_cast<uint64_t *>(keys_);\n    }\n\n    uint64_t *\n    get_values_ptr_() const\n    {\n        return reinterpret_cast<uint64_t *>(values_);\n    }\npublic:\n\n    Hashtable(size_t nreserve = 1024)\n        : ndata_(nreserve)\n        , keys_(zalloc8_(ndata_))\n        , values_(zalloc8_(ndata_))\n    {}\n\n    void\n    insert(uint64_t k, uint64_t v)\n    {\n        uint64_t *keys = get_keys_ptr_();\n        uint64_t *values = get_values_ptr_();\n        uint64_t i = k % ndata_;\n        while (keys[i] != k) {\n            if (!keys[i]) {\n                keys[i] = k;\n                break;\n            }\n            i = (i + 1) % ndata_;\n        }\n        values[i] = v;\n    }\n\n    uint64_t\n    get(uint64_t k) const\n    {\n        uint64_t *keys = get_keys_ptr_();\n        uint64_t *values = get_values_ptr_();\n        uint64_t i = k % ndata_;\n        while (keys[i]) {\n            if (keys[i] == k) {\n                return values[i];\n            }\n            i = (i + 1) % ndata_;\n        }\n        return 0;\n    }\n\n    ~Hashtable()\n    {\n        free(get_keys_ptr_());\n        free(get_values_ptr_());\n    }\n};\n\ntypedef uint64_t Address;\n\nclass ERC20\n{\n    uint64_t totalSupply() { return 0; }\n    uint64_t balanceOf(Address who) { return 0; }\n    bool transfer(Address from, Address to, uint64_t value) { return false; }\n};\n\nclass MyToken : public ERC20\n{\n    uintptr_t balances_;\n    uint64_t totalSupply_;\n\n    Hashtable *\n    get_balances_() const\n    {\n        return reinterpret_cast<Hashtable *>(balances_);\n    }\n\npublic:\n    MyToken(Address creator, uint64_t initial_balance)\n        : balances_(reinterpret_cast<uintptr_t>(create<Hashtable>()))\n        , totalSupply_(initial_balance)\n    {\n        get_balances_()->insert(creator, initial_balance);\n    }\n\n    uint64_t totalSupply() const\n    {\n        return totalSupply_;\n    }\n\n    uint64_t balanceOf(Address who) const\n    {\n        return get_balances_()->get(who);\n    }\n\n    bool transfer(Address from, Address to, uint64_t value)\n    {\n        Hashtable *b = get_balances_();\n        if (!to) {\n            return false;\n        }\n\n        const uint64_t prev_from_amount = b->get(from);\n        if (prev_from_amount < value) {\n            return false;\n        }\n\n        const uint64_t prev_to_amount = b->get(to);\n        if (UINT64_MAX - prev_to_amount < value) {\n            return false;\n        }\n\n        b->insert(from, prev_from_amount - value);\n        b->insert(to, prev_to_amount + value);\n        return true;\n    }\n\n    ~MyToken()\n    {\n        get_balances_()->~Hashtable();\n    }\n};\n"
    }
};

if (File.getFileNames().length == 0) {
    for (filename in start_files_data) {
        if (!start_files_data.hasOwnProperty(filename)) {
            continue;
        }

        var file = new File(filename);
        file.code = start_files_data[filename].code;
    }
}

File.getFiles().forEach(function (f) {
    f.maintainId = null;
});


function updateFilesList() {
    ul = $('#files ul');
    ul.children().remove();

    File.getFiles().forEach(function (file) {
        let filename = file.name;

        listElement = $('<li>' + filename + '</li>');
        if (selectedFile == filename)
            listElement.addClass('current');
        ul.append(listElement);

        listElement.on('click', function () {
            let doClearSelection = (filename != selectedFile);
            fileSelected(filename);
            if (doClearSelection)
                editor.session.getSelection().clearSelection();
        });
    });
}

function displayCompiled(text, isSmartContract) {
    let area = $('.compiled-area');
    if (text == null) {
        area.html("");
        area.addClass('unfilled');

        $('.deploy-button, .test-button').fadeOut();
    } else {
        area.html(text.replace(/\r?\n/g, '<br/>'));
        area.removeClass('unfilled');

        $('.deploy-button, .test-button').fadeIn();
    }
}

function fileSelected(name) {
    selectedFile = name;
    if (location.hash != "#" + name) {
        location.hash = "#" + name;
    }
    updateFilesList();

    var file = new File(name);
    var format = file.format;

    editor.session.setMode(highlight_formats[format]);
    console.log(format);

    var cacheCompiled = file.compiled;
    editor.setValue(file.code);
    file.compiled = cacheCompiled;
    displayCompiled(file.compiled && file.compiled.disassembler && file.compiled.disassembler.stdout);

    let toolbox = $('.contract-toolbox');
    if (file.isSmartContract && file.maintainId) {
        toolbox.fadeIn();
    } else {
        toolbox.fadeOut();
    }
}

function displayCompilationWaste(compilationResponse) {
    let data = []
    if (compilationResponse) {
        if (compilationResponse.llvmExecution)
            data.push({
                title: "LLVM:",
                stdout: compilationResponse.llvmExecution.stdout,
                stderr: compilationResponse.llvmExecution.stderr
            });
        if (compilationResponse.translatorExecution)
            data.push({
                title: "LLVM -> RBVM Translator:",
                // stdout : compilationResponse.translatorExecution.stdout,
                stderr: compilationResponse.translatorExecution.stderr
            });
    }
    InterfaceConsole.setData(data);
}

function compileCurrentFile() {
    InterfaceConsole.clear();
    displayCompiled(null);
    var file = new File(selectedFile);
    file.maintainId = null;
    fileSelected(selectedFile);
    $.ajax("/api/compile", {
        data: {
            'code': file.code,
            'format': file.format,
            'smart': file.isSmartContract ? "smart_contract" : null
        },
        method: "POST"
    }).done(function (result) {
        if (result && result.bytecode) {
            console.log(result);
            file.compiled = {
                bytecode: result.bytecode,
                disassembler: result.disassemblerExecution
            };
            displayCompiled(file.compiled.disassembler && file.compiled.disassembler.stdout);
        }
        displayCompilationWaste(result);
    });
}

function runBytecodeInOneShotMode(bytecode) {
    $.ajax("/api/run", {
        data: {
            'bytecode': bytecode
        },
        method: "POST"
    }).done(function (result) {
        if (result) {
            console.log(result);
            InterfaceConsole.setData(result);
        }
    });
}

function runBytecodeInMaintainMode(bytecode, file) {
    $.ajax("/api/run-maintain", {
        data: {
            'bytecode': bytecode
        },
        method: "POST"
    }).done(function (result) {
        console.log(result);
        file.maintainId = result.maintainId;
        fileSelected(selectedFile);
        InterfaceConsole.setData(result.executionResponse);
    });
}

function runBytecode(file) {
    let bytecode = file && file.compiled && file.compiled.bytecode;
    if (!bytecode)
        return;

    InterfaceConsole.clear();

    if (file.isSmartContract) {
        runBytecodeInMaintainMode(bytecode, file);
    } else {
        runBytecodeInOneShotMode(bytecode, file);
    }
}


function callSmartMethod(action, attrs) {
    let callString = action + '\n' + attrs.join(' ');
    let file = new File(selectedFile);
    $.ajax("/api/communicate-maintain", {
        data: {
            'id': file.maintainId,
            'line': callString
        },
        method: "POST"
    }).done(function (result) {
        console.log(result);
        InterfaceConsole.appendData(result);
        if (result.stderr) {
            file.maintainId = null;
            fileSelected(file.name);
        }
    });
}


function tryCreateFile(name) {
    var alreadyExists = File.getFileNames().indexOf(name) >= 0;
    if (alreadyExists) {
        swal({
            title: "File " + name + " already exists",
            icon: 'warning'
        });
    } else {
        let file = new File(name);
        if (!file.format || !highlight_formats.hasOwnProperty(file.format)) {
            file.delete();
            swal({
                title: 'Wrong file extension',
                text: 'Only .c and .cpp formats are supported.',
                icon: 'warning'
            });
        } else {
            fileSelected(name); // this function calls updateFileList()
        }
    }
}


function selectFileAccordingToLocationHash() {
    let hash = location.hash;
    hash = hash && hash.substr(1);

    let list = File.getFileNames();
    console.log(list + " " + hash);
    if (list.indexOf(hash) >= 0) {
        if (selectedFile != hash)
            fileSelected(hash);
    } else {
        location.hash = "#" + selectedFile;
    }
}


$('.compile-button').on('click', function () {
    if (selectedFile) {
        compileCurrentFile();
    }
});

$('.test-button').on('click', function () {
    if (selectedFile) {
        let file = new File(selectedFile);
        runBytecode(file);
    }
});

$('.deploy-button').on('click', function () {
    swal("You're an early bird!", "Sorry, the Fantom Network is still in development.");
});

$('.create-button').on('click', function () {
    swal({
        content: {
            element: 'input',
            attributes: {
                placeholder: 'Type file name here'
            }
        },
        title: 'Create new file',
        text: 'Use .c or .cpp extension.',
        buttons: {
            cancel: true,
            confirm: true
        }
    }).then(function (value) {
        if (value)
            tryCreateFile(value);
    });
});

editor.session.on('change', function () {
    if (selectedFile != null) {
        var file = new File(selectedFile);
        file.code = editor.getValue();
        file.compiled = null;
        displayCompiled(null);
    }
});

$(window).on('hashchange', function () {
    console.log("Changed");
    selectFileAccordingToLocationHash();
});


(function INIT_CONTRACT_TOOLBOX() {
    function getArgNames(element) {
        let attrs = element.dataset.args;
        if (!attrs)
            return 0;
        return attrs.split('|');
    }

    $('.contract-toolbox .method-button').on('click', function (ev) {
        if (!$(ev.target).hasClass('method-button'))
            return;

        let action = ev.target.dataset.action;
        let numArgs = getArgNames(ev.target).length;
        let attrs = new Array(numArgs);
        for (let i = 0; i < numArgs; i++) {
            attrs[i] = $(ev.target).find("input[data-index='" + i + "']").val();
            if (attrs[i] === "") {
                swal({
                    title: 'Fill all arguments',
                    icon: 'warning'
                });
                return;
            }
        }
        console.log(attrs);
        callSmartMethod(action, attrs);
    });

    $('.contract-toolbox .method-button').each(function () {
        let self = this;
        $(self).on('keypress', function (ev) {
            if (ev.which == 13) {
                $(self).click();
            }
        });
    });

    $('.contract-toolbox .method-button').each(function () {
        let argNames = getArgNames(this);
        for (let i = 0; i < argNames.length; i++) {
            $(this).children('span')
                .append($('<input type="text" data-index="' + i + '" placeholder="' + argNames[i] + '"/>'));
            if (i != argNames.length - 1) {
                $(this).children('span').append($('<span>, </span>'));
            }
        }
    });
})();


selectFileAccordingToLocationHash();
if (!selectedFile) {
    fileSelected(File.getFileNames()[0]);
}

// TODO think about reactive architecture
