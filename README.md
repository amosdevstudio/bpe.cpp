# bpe.cpp

A simple, multithreaded and ⚡BLAZINGLY FAST⚡ Byte Pair Encoder written in c++ with easy to use python bindings.

### Pre-trained tokenizers:

Check out the [releases tab](https://github.com/amosdevstudio/bpe.cpp/releases/tag/pre-trained) for pre-trained tokenizers.

### How to fit to custom data:
I provided a fit.cpp file to easily fit a custom tokenizer to custom data.
You will need:
- A list of letters that will split the words in your custom data (like a regex);
- The path to a text file (preferrably massive) that contains the data to fit on;
- The vocab size for your new tokenizer (should be bigger than 256);

The algorithm used by the fit method is the iterative greedy BPE algorithm proposed in [this paper](https://arxiv.org/abs/2306.16837).
The algorithm runs (at least for now) on only one thread, but is more than fast enough for my use case.

I trained a tokenizer on 200 books from the [gutenberg dataset](https://shibamoulilahiri.github.io/gutenberg_dataset.html) (scraped from the [gutenberg project](https://www.gutenberg.org/)).
The file was around 67 Mib (70M characters), and a vocab size of 131072 (2^17). The training took about 1 minute and 40 seconds.


to use that, you can just compile the fit.cpp file and the bpe.cpp file.
Compiler optimizations are recommended.
Example (gcc):
```
gcc src/bpe.cpp src/fit.cpp -O3 -o fit
```
Then run the program:
```
./fit
```

### How to test the encoder:
I also provided an encode.cpp file, that loads a `tokenizer.bpe` file in the current directory and uses it to encode whatever you put into it.
Again, compile the encode.cpp file with the bpe.cpp file and run the program.
Example (gcc):
```
gcc src/encode.cpp src/bpe.cpp -O3 -o encode
```
And run:
```
./encode
```

### How to use the python wrapper:
Fitting with the python wrapper is possible but not recommended.

To use the python wrapper, install the pybind11 package. Follow the [installation guide](https://pybind11.readthedocs.io/en/stable/installing.html) for further information.
Then, run this command:
```
python3 setup.py build_ext --inplace
```
After running this command, there should be a build directory and a .so (or .pyd on windows) file in your current directory.
I provided an encode.py file to test the python wrapper. (See the docs below for the available methods)

> [!NOTE]
> Decoding tokens that encode unicode characters using the python wrapper is not fully supported.
> As detailed in the [pybind11 Issue #591](https://github.com/pybind/pybind11/issues/591), pybind can make the application crash if it finds an invalid unicode character.
> That is a problem especially when trying to decode (valid) utf-8 tokens one at a time, since the unicode byte sequence might be chopped off and become invalid.
> To fix this, you can wrap the decode function in a try except block and handle the error from there (like in the demo I provided).

## Documentation

### .bpe files:

.bpe files are the files used by bpe.cpp to save and load tokenizers (JSON support coming soon).
Here is an example .bpe file:
```
' -_!?.:;,<>()[]{}=
258
32 32
256 256
```
The first line is the list of characters that split the text into words.
The second line specifies the vocab size (minimum 257).
The next lines specify the merges of the byte pair encoder, in this case:
`32 32` means "Merge char number 32 (a space) with char number 32 (another space)".
`256 256` means "Merge token number 256 (the 2 spaces created earlier) with token 256 (the same token)".

The vocab for this bpe consists in all the unicode characters (From 0 to 255), plus token
256 (= 2 spaces) and
257 (= 4 spaces).

I hope the explanation was clear.

bpe.cpp doesn't support JSON yet (like tiktoken), but it probably will in the future (hopefully after my summer vacation :P).

### Class methods:

```void BPE::BPE();```

The constructor for the BPE.

```void BPE::Load(const std::string& path);```

Loads a .bpe file.
This function takes in the path to a custom .bpe file and loads it to a BPE class.

```void BPE::LoadSplitLetters(const std::string& splitLetters);```

Loads the split letters.
This funtion takes in a list of characters (string) and stores it in the BPE as an unordered set for later use.

```void BPE::Fit(const size_t vocabSize, const std::string& path);```

Fit the BPE to a text file.
This function takes in 2 arguments:
 - The vocab size, the number of tokens used by the encoder.
 - The path to the text file for custom fitting

```void BPE::Save(const std::string& path) const;```

Saves the BPE to a .bpe file.
This function takes in a path for saving the BPE to a .bpe file.

```TokenList BPE::Encode(const std::string& text) const;```

Encodes the given string to a linked list of tokens.

```std::string BPE::Decode(const TokenList& tokens) const;```

Decodes the given linked list of tokens back to a string.

```std::string BPE::DecodeFromVector(const std::vector<uint32_t>& tokens) const;```

Decodes the given vector of tokens back to a string (used in the python wrapper).

```std::vector<uint32_t> BPE::EncodeToVector(const std::string& text) const;```

Encodes the given string to a vector of tokens (used in the python wrapper).

## The boring stuff:
I made this because I wanted to train my own Byte Pair Encoder on the Gutenberg dataset. I started by using Andrej Karpathy's minbpe, but my PC is simply too slow.
I then realized that the problem was python, so I switched to pypy for better performance. Although it got much better, it was still nowhere near what I needed.
So I realized that rewriting it in a lower level language like c++ would give me finer optimization control. And that's exactly what happened.

What would take days on pypy now takes just a few minutes (and much less electricity) in c++. If you want to try out my own pre-trained tokenizer trained on a subset of the gutenberg dataset, it's on the [releases page](https://github.com/amosdevstudio/bpe.cpp/releases/tag/pre-trained).


I am fairly new to c++ programming, so don't kill me if my code is garbage. Any feedback is very appreciated :).

Also, feel free to rewrite everything in Rust if you want to. I know it's tempting.

I am not a native english speaker, so feel free to correct any errors or help me word things more clearly.
