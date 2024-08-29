from pybpe import BPE

bpe = BPE()
bpe.load("tokenizer.bpe")

while True:
    encoded = bpe.encode(input("Text:\n"))
    decoded_list = []
    for token in encoded:
        try:
            decoded_list.append(bpe.decode([token]))
        except: # UnicodeDecodeError
            decoded_list.append('� ')

    decoded = bpe.decode(encoded)

    print(encoded)
    print(decoded_list)
    print(decoded)
