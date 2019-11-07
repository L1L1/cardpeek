Cardpeek
========

## About

_([Version française](README.fr.md))_

Cardpeek is a Linux/Windows/Mac OS X tool to read the contents of ISO7816 smart cards. It features a GTK GUI to represent card data in a tree view, and is extendable with a scripting language (LUA).

The goal of this project is to allow smart card owners to be better informed about what type of personal information is stored in these devices.

The tool currently reads the contents of :

* [EMV](doc/emv.md) Pin and Chip cards, including NFC ones - _[screenshot](doc/sample-emv.jpg)_
* [Navigo](doc/navigo.md) (Paris), MOBIB (Belgium), RavKav (Israel) and other public transport cards of the Calypso family - _[screenshot](doc/sample-navigo.jpg)_
* The French health card "Vitale 2"
* Electronic/Biometric passports in BAC security mode
* GSM SIM cards (but not USIM data)
* The Belgian eID card
* Driver tachograph cards
* OpenPGP cards (beta)

It can also read the following cards with limited interpretation of data:
* Some Mifare cards (such as the Thalys card)
* Moneo, the French electronic purse

## Build

**!!! Produced binaries do not run yet - See [issue #1](https://github.com/ipamo/cardpeek/issues/1) !!!**

- [Build instructions for Debian](doc/build-debian.md), either for the local Debian host, or for cross-compilation to Windows using mingw-w64.
- [Build instructions for Windows](doc/build-windows.md), using msys2.
- [Specific instructions for FreeBSD](doc/build-freebsd.md) in case of errors.

## Usage

The [Reference Manual](doc/cardpeek_ref.en.pdf) provides detailed usage instructions.

## Authors

Written initially by Alain Pannetrat under the [GNU General Public License, version 3](COPYING), with the additional exemption that compiling, linking, and/or using OpenSSL is allowed.

More info here: http://pannetrat.com/Cardpeek/
