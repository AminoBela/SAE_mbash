# Rapport - SAE 3.03 : Réseau et Application Serveur

**Auteurs :** Amin Belalia Bendjafar et Clément De Wasch  
**Date :** 17 Janvier 2025

## Table des matières

1. [Introduction](#introduction)
2. [Partie 1 : mbash](#partie-1-mbash)
3. [Partie 2 : Serveur de package Debian](#partie-2-serveur-de-package-debian)
4. [Conclusion](#conclusion)

---

## Introduction

Ce rapport présente la réalisation de **mbash**, une version simplifiée de Bash, et la mise en place d'un **serveur Debian** pour distribuer mbash sous forme de package.

---

## Partie 1 : mbash

**Objectifs :**
- Créer un shell minimaliste avec des fonctionnalités comme `cd`, `pwd`, `exit`, gestion des variables d’environnement et de l'historique, exécution en arrière-plan avec `&`, etc.

**Fonctionnalités :**
- Commandes de base (`cd`, `pwd`, `exit`).
- Opérateurs logiques (`&&`, `||`, `;`).
- Gestion des guillemets et historique des commandes.

**Analyse technique :**
- Utilisation d'un automate à états finis pour analyser les commandes.
- Exécution via `execve` pour lancer les commandes externes.

---

## Partie 2 : Serveur de package Debian

**Objectifs :**
- Créer un dépôt Debian pour distribuer **mbash** via **apt**.

**Réalisation serveur :**
- Création d'un dépôt sur le serveur avec structure appropriée.
- Signature des paquets avec GPG.
- Mise en place d'un serveur HTTP avec Apache2 pour héberger le dépôt.

**Réalisation client :**
- Ajout du dépôt dans `/etc/apt/sources.list`.
- Installation de mbash avec `apt install`.

---

## Conclusion

Le projet a permis d'explorer la création d’un shell simple (mbash) et la gestion d'un dépôt Debian pour la distribution de paquets logiciels. L'intégration des commandes de base et la gestion des paquets via **apt** assurent une solution complète et fonctionnelle.

