# bpe.cpp

A simple, multithreaded and ⚡BLAZINGLY FAST⚡ Byte Pair Encoder written in c++ with easy to use python bindings.

### Pre-trained tokenizers:

Check out the releases tab for pre-trained tokenizers.

## Documentation

### How to fit to custom data:

To fit to custom data I provided a fit.cpp file to easily train a tokenizer.
Fitting is reccomended in c++ as it is faster than using the python wrapper (although the difference is minimal).
The file generated in c++ can easily be later loaded in python for use with other python libraries (such as pytorch, tensorflow and others.)

Compile the fit.cpp file with the bpe.cpp file and link with the pcre2-8 library.
Compiler optimizations are recommended as they can significantly speed up the program (up to 200% on my machine)

Example (gcc):
```
gcc src/fit.cpp src/bpe.cpp -O3 -lpcre2-8 -o fit
```
Then run the program:
```
./fit
```
It will ask a bunch of questions and finally generate a `tokenizer.bpe' file.

### Usage in python:

For use in python, first install the pybind11 package.
You can either use pip or your distribution's package manager (if you can).
```
pip install pybind11
```
If you are using pyenv, using pip might not work.
Read the [pybind11 docs](https://pybind11.readthedocs.io/en/stable/installing.html) for further info.

Then, set up the python module.
```
python3 setup.py build_ext --inplace
```
This command will generate a .so file (on Mac and Linux) or a .pyd file (on Windows) and an extra build directory.
The build directory contains intermediate files and can be ignored.
Now you can simply move the .so or .pyd file to your project directory and use it as pybpe.

```
from pybpe import BPE

bpe = BPE()
bpe.load("tokenizer.bpe")
encoded = bpe.encode("Hello world")
decoded = bpe.decode(encoded)

print(encoded, '\n', decoded)
```
Although fitting with the python wrapper is possible, it is reccomended to fit with c++ for performance reasons.

### .bpe files:

.bpe files are the files used by bpe.cpp to save and load tokenizers (JSON support coming soon).
Here is an example .bpe file:
```
'(?i:[sdmt]|ll|ve|re)|[^\r\n\p{L}\p{N}]?+\p{L}+|\p{N}{1,3}| ?[^\s\p{L}\p{N}]++[\r\n]*|\s*[\r\n]|\s+(?!\S)|\s+
258
32 32
256 256
```
The first line is the regex pattern used by the tokenizer to split the words.
The second line specifies the vocab size (minimum 256).
The next lines specify the merges of the byte pair encoder, in this case:
`32 32` means "Merge char number 32 (a space) with char number 32 (another space)".
`256 256` means "Merge token number 256 (the 2 spaces created earlier) with token 256 (the same token)".

The vocab for this bpe consists in all the unicode characters (From 0 to 255), plus token
256 (= 2 spaces) and
257 (= 4 spaces).

I hope the explanation was clear.

bpe.cpp doesn't support JSON yet (for tiktoken), but it probably will in the future (hopefully after my summer vacation :) ).

### Class methods:

```void BPE::Load(const std::string& path);```

Loads a .bpe file.
This function takes in the path to a custom .bpe file and loads it to a BPE class.

```void BPE::LoadRegex(const std::string& regexText);```

Loads a custom regex
This funtion takes in a PCRE2 regex pattern, compiles it, and stores it in the BPE for later use.

```void BPE::Fit(const size_t vocabSize, const std::string& dataPath, const size_t numThreads);```

Fit the BPE to a text file.
This function takes in 3 arguments:
 - The vocab size, the number of tokens used by the encoder.
 - The data path, the path to the text file for custom fitting
 - numThreads, the number of threads to use.

```void BPE::Save(const std::string& path);```

Saves the BPE to a .bpe file.
This function takes in a path for saving the BPE to a .bpe file.

```std::vector<unsigned int> BPE::Encode(const std::string& text);```

Encodes the given string to a vector of tokens.

```std::string BPE::Decode(const std::vector<unsigned int>& tokens);```

Decodes the given vector of tokens back to a string.

## The boring stuff:
I made this because i wanted to train my own Byte Pair Encoder on the Gutenberg dataset. I started by using Andrej Karpathy's minbpe, but my PC is simply too slow.
I then realized that the problem was python, so i switched too pypy for better performance, and it got much better, but still nowhere near what i needed.
So I realized that rewriting it in a lower level language like c++ would give me finer optimization control. And that's exactly what happened.

What would take days on pypy now takes just a few hours (and much less electricity). If you want to try out my own pre-trained tokenizer trained on a subset of the gutenberg dataset, it's on the releases page.


I am fairly new to c++ programming, so don't kill me if my code is garbage. Any feedback is very appreciated :).

Also, feel free to rewrite everything in Rust if you want to. I know it's tempting.

I am not a native english speaker, so feel free to correct any errors or help me word things more clearly.
