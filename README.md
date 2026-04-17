# pngprobe

A command-line tool that scans PNG files for hidden data in `tEXt` metadata chunks.
If the hidden data is a base64-encoded ZIP containing a PyTorch tensor, it extracts
and prints the float32 values.

**This is a learning project.** Built to understand the PNG format from first principles
and explore how steganography can be used to carry neural steering vectors.

## Why

This tool was built after coming across the [Synthbody](https://lnkd.in/d-eGP5Nc)
project by Nicolò Tarascio and Gaia Riposati. They embedded a neural steering vector
inside a PNG image using `tEXt` chunk injection, a technique they call Neural
Steganography. They showed that when the vector is manually injected into layer 16
of LLaMA 3.2 3B Instruct, the model exhibits stress-like behavior.

The vector does not activate by just showing the image to a model. It requires
direct access to the model weights and manual injection into a specific layer.
`pngprobe` extracts and inspects that vector without using their lab or tools.

## Build

    sudo apt install libzip-dev
    make

Dependencies: zlib, libzip.

## Usage

Scan only:

    ./pngprobe input.png

Scan and save the extracted ZIP to disk:

    ./pngprobe input.png output.zip

## How it works

PNG files are made of blocks called chunks. One type, `tEXt`, stores plain text
as `key + null byte + value`. This tool walks every chunk, detects long
base64-encoded values, decodes them, and checks whether the result is a ZIP
archive containing a PyTorch tensor (`data/0`).

The 90% base64 detection threshold accounts for padding and newlines that are
valid in base64 strings but not part of the alphabet.

### Data flow

    PNG file
        |
        v
    [signature check] --> not a PNG? stop.
        |
        v
    [chunk walker]
        |
        +--> IDAT, IHDR, ...  ignored
        |
        +--> tEXt
                |
                v
            [CRC32 check] --> mismatch? warn and continue
                |
                v
            [base64 probe] --> not base64? skip
                |
                v
            [base64 decode]
                |
                v
            [ZIP check] --> not a ZIP? skip
                |
                v
            [ZIP open] --> no data/0? skip
                |
                v
            [float32 reader]
                |
                v
            3072 x float32
            (steering vector)

## Example output

    === Scanning PNG chunks ===

    tEXt  key="NEURO_VECTOR"  value=<19096 bytes>
      -> Looks like base64, decoding...
      -> Decoded: 14320 bytes
      -> It's a ZIP file!

    === ZIP contents (7 entries) ===
      [ 0] cortisol/data.pkl  (583 bytes)
      [ 4] cortisol/data/0    (12288 bytes)
            ^ tensor data file

    === Tensor: 3072 float32 values ===

      [   0]  +0.009386
      [   1]  +0.008989
      [   2]  -0.029922
      ... (3052 more values)

    tEXt  key="v_source"  value="cortisol.pt"
    tEXt  key="v_color1"  value="#d21a4e"
    tEXt  key="v_color2"  value="#40eb99"

## References

PNG specification: https://www.w3.org/TR/png/

PyTorch serialization format: https://pytorch.org/docs/stable/generated/torch.save.html

ZIP format: https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT

## Connect with me

[LinkedIn](https://www.linkedin.com/in/francescopl/) · [Kaggle](https://www.kaggle.com/francescopaolol)
