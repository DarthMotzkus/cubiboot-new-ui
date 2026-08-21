# Banner converter

Turns an ordinary image into GameCube banner artwork. Use it to give a homebrew app the
`opening.bnr` that makes cubiboot list it with a banner instead of as a folder — see
[Homebrew apps](../../docs/settings.md#homebrew-apps).

It builds either of two things, picked from a menu when you run it:

| | Output | Use it for |
|---|---|---|
| **1** | `.bmp` | Importing into an existing `opening.bnr` with **GCRebuilder** |
| **2** | `opening.bnr` | A complete banner file, for an app folder that ships its own |

## Usage

```sh
pip install Pillow
python run.py
```

Put your images in the same folder as `run.py`, run it, and pick an option. Results land in
`output/`.

For option 2 it asks for a title, an author and a description, and writes
`output/<name>/opening.bnr`. Drop the app's `default.dol` next to that file and the folder is
ready to copy to the card.

Any source size and any common format works — the image is fitted into the 96×32 the console
expects rather than squashed. `--fit contain` (default) keeps the whole image and pads the
leftover, `cover` fills the slot and crops the overflow, `stretch` distorts to fit.

Both modes take flags so a whole catalog can be built without the prompts:

```sh
python run.py --mode bnr --fit contain --title "My App" --author "Me" --description ""
```

## Sizing

The banner slot is 96×32 — a 3:1 rectangle. With `contain`, a source's aspect ratio decides
how much of the slot it fills, because the width is always used up first:

| Source aspect | Rows filled, of 32 |
|---|---|
| 10:1 | 10 |
| 8:1 | 12 |
| 6.9:1 | 14 — what the stock GameCube banners use |
| 4.8:1 | 20 |
| 3:1 | 32, edge to edge |

So a wordmark that looks thin on screen wants a *taller* source file, not a wider one.

## Where this comes from

The canonical copy lives at
[DarthMotzkus/banner-converter-gc](https://github.com/DarthMotzkus/banner-converter-gc),
which also ships a disc-number SVG template and the GCRebuilder build. This copy is vendored
so the tool sits next to the feature that needs it; take fixes upstream first.
