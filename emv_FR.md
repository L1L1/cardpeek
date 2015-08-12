# How can I read the contents of my EMV bank card? #

![http://cardpeek.googlecode.com/files/sample-emv.jpg](http://cardpeek.googlecode.com/files/sample-emv.jpg)

The "emv" script in **cardpeek** provides an analysis of EMV banking cards used across the
world.

# Notes #

This script will ask you if you want to issue a Get Processing Option (GPO)
command for each application on the card. Since some cards have several applications
(e.g. a national and an international application), this question may be asked twice or
more. This command is needed to allow access to some information in the card. Issuing this command will also increase an internal counter inside the card (the ATC).

# Notes on privacy #

You will notice that many of these bank cards keep a \transaction log" of the last
transactions you have made with your card. Some banks cards keep way over a hundred
transactions that are freely readable, containing the date, the amount and the country of the transaction, which brings up some privacy issues. Why do banks need to keep so much information in the card?

The security elements in the card, such as the PIN and the cryptographic keys are fully protected in the card and cannot be read.