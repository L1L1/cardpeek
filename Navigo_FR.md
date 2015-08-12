# Comment lire le contenu de son "Passe Navigo" ? #

![http://cardpeek.googlecode.com/files/sample-navigo.jpg](http://cardpeek.googlecode.com/files/sample-navigo.jpg)

Le script "calypso" inclu dans **cardpeek** permet de lire le contenu des cartes "Navigo" utilisées en région parisienne. Cet outil offre une analyse améliorée des "journaux d'évènements" de la carte affichant notamment le nom des stations de métro/RER où la carte a été utilisée. Cet outil a été testé avec succès sur les cartes Navigo Découverte, Navigo et Navigo Intégrale.

# Notes #

Il faut utiliser **cardpeek** avec un lecteur de carte à puce à contact pour lire le contenu d'une carte "navigo". En effet, les cartes "navigo" ne peuvent pas êtres lues avec des lecteurs sans contact _classiques_ (car elles utilisent un protocole de communication radio qui n'est pas totalement compatible avec l'ISO 14443 B).

# Protection de la vie privée #

Ces cartes de transport conservent un "journal d'évènement" décrivant les 3 dernières validations effectuées par le porteur de la carte. Ce journal, qui peut poser un risque pour la protection de la vie privée, n'est pas protégé en lecture.
Par contre le nom du porteur de la carte n'est pas, selon nos analyses, accessible en lisant la carte. En revanche, c'est le cas pour la carte MOBIB utilsée à Bruxelles.