# Camus

[![Build and Deploy](https://github.com/briandowns/camus/actions/workflows/main.yml/badge.svg)](https://github.com/briandowns/camus/actions/workflows/main.yml/badge.svg)

Camus is a small dev server for projects that use Make.

## Usage

Run Camus so that changes to the code base will trigger a rebuild.

```sh
camus 
```

Tell Camus the target you want ran is "test". Camus will run `make test` when it detects changes.

```sh
camus -t test
```

## Installation

```sh
make install
```

## Contributing

Please feel free to open a PR!

## License

camus source code is available under the BSD 2 clause [License](/LICENSE).

## Contact

[@bdowns328](http://twitter.com/bdowns328)
