handlePOST()
│
├── vérifier erreurs
│
├── if (isCGI)
│       return executeCGI()
│
└── else
        saveToFile()

Si method == POST
    Si body vide → erreur 400
    Sinon
        créer fichier
        écrire body dedans
        retourner 201 Created

Si extension du fichier == .py (ou config CGI)
    exécuter script
    envoyer body au script
    récupérer sortie
    retourner sortie

Si c est vers un script (PHP, Python…) :

    Le serveur crée un environnement CGI

    1 - Il passe les headers en variables d’environnement

    2 - Il envoie le body au script via stdin

    3 - Le script traite les données

    4 - Le script génère une réponse (HTML, JSON…)

    5 - Le serveur renvoie ça au client

3️⃣ Qu’est-ce que POST retourne ?

✅ POST retourne toujours :

    HTTP/1.1 STATUS_CODE
    Headers

    Body

    Mais le status change selon le résultat.

    ✅ Cas normal (succès)
    🟢 200 OK

    Action réussie.

    🟢 201 Created

    Nouvelle ressource créée (upload par exemple).

    🟢 204 No Content

    Réussi mais pas de body.


    --------------------------------------------------------

---


# 🧠 4️⃣ Créer les variables d’environnement (TRÈS IMPORTANT)

👉 Tu prépares des infos comme :

* REQUEST_METHOD = POST
* CONTENT_LENGTH = taille du body
* CONTENT_TYPE = type de données

---

## ❓ Pourquoi on crée ces variables ?

Parce que le script CGI n’a PAS accès à ton serveur.

Donc :

> Le serveur doit lui donner les infos de la requête.

---

## 🎭 Image simple

Le serveur dit au script :

> “Voilà ce que le client t’a envoyé :
> méthode = POST
> taille = 27
> type = form-data”

---

## ❓ Où sont utilisées ces variables ?

👉 **Dans le script lui-même**

Exemple en Python :

* il lit la méthode
* il lit combien de bytes lire
* il traite les données

Donc :

> Les variables d’environnement = langage de communication entre serveur et script

---

# 🥇 5️⃣ Créer un canal de communication (pipes)

👉 Tu crées 2 canaux :

1. envoyer le body au script
2. lire la réponse du script

Pourquoi ?

> Parce que le script est un autre programme séparé
> tu dois communiquer avec lui

---

# 🥈 6️⃣ Créer un processus enfant

👉 Le serveur se divise en deux :

* parent (serveur)
* enfant (script CGI)

Pourquoi ?

> Pour que le script s’exécute séparément
> sans bloquer le serveur

---

# 🥉 7️⃣ Lancer le script (execve)

👉 L’enfant devient :

```
python3 script.py
```

Pourquoi ?

> Parce que ton serveur ne sait pas exécuter Python
> il lance l’interpréteur

---

# 🧃 8️⃣ Envoyer le body (POST)

👉 Le serveur envoie les données au script

Pourquoi ?

> Parce que POST contient des données
> et le script doit les lire

---

# 📥 9️⃣ Lire la réponse du script

👉 Le script écrit :

```
Content-Type: text/html

<html>...</html>
```

Pourquoi ?

> Parce que le script produit la réponse finale

---

# 🧾 🔟 Construire la réponse HTTP

👉 Ton serveur prend ce que le script a donné
et l’envoie au client

Pourquoi ?

> Parce que le client ne parle qu’en HTTP

---

# 🧠 Résumé ultra simple

```text
Client → Server → Script → Server → Client
```

* serveur lance script
* script traite
* serveur renvoie résultat

---

# 🎯 Résumé des rôles

| Élément       | Rôle                    |
| ------------- | ----------------------- |
| serveur       | organise tout           |
| variables env | donnent infos au script |
| script CGI    | traite les données      |
| pipes         | communication           |
| execve        | lance le script         |

---

# 🔥 La clé à retenir

👉 Le serveur ne “comprend” pas le script
👉 Il **lance** un programme externe
👉 Il lui donne les infos via environment
👉 Il communique via pipes

---

Franchement Nouss…
si tu comprends cette partie, tu viens de comprendre comment fonctionne **Apache / Nginx CGI en vrai** 😌🔥

Tu veux que je te fasse un mini schéma mental ultra visuel (genre dessin ASCII) pour que ça reste dans ta tête pour toujours ?
