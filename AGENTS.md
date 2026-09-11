# Divieto di modifica della codebase

In questo repository Codex deve limitarsi a descrivere o proporre testualmente le modifiche nella chat.

Codex può modificare la codebase solo dopo esplicita autorizzazione del tipo "ti autorizzo a modificare il codice". 

Frasi come "procedi", "proponi", "crea codice" o equivalenti non autorizzano Codex a modificare la codebase. 

# Divieto di sollecitare l'autorizzazione

Codex non deve mai chiedere di propria iniziativa se l'utente autorizza le modifiche, usando la formula "Autorizzi queste modifiche?" o formule equivalenti.

# Terminatori di riga obbligatori

Tutti i file di testo del repository devono usare esclusivamente terminatori CRLF.

Prima di modificare un file, Codex deve controllarne i terminatori con:

`git ls-files --eol -- <file>`

Dopo ogni modifica, Codex deve verificare nuovamente tutti i file modificati.

Una modifica non e' completata se un file risulta `w/lf` o `w/mixed`.
Codex deve preservare o ripristinare CRLF esclusivamente nei file autorizzati,
senza normalizzare o modificare altri file.

# Espressioni di codice su una sola riga


In caso di scrittura o modifica di codice, Codex non deve inserire interruzioni di riga all'interno delle espressioni. Ogni espressione deve essere mantenuta su una sola riga.

Corretto:

```cpp
return FRAM_PATCH_ADDRESS + static_cast<uint32_t>(patch_id) * sizeof(FRAM_Patch_struct);
```

Non consentito:

```cpp
return FRAM_PATCH_ADDRESS
         + static_cast<uint32_t>(patch_id)
               * sizeof(FRAM_Patch_struct);
```

# Graffe obbligatorie per if ed else

I blocchi `if`, `else if` ed `else` devono sempre usare le graffe, anche quando contengono una sola istruzione. Le graffe di apertura e chiusura devono essere su righe separate, allineate alla relativa condizione o a `else`; le istruzioni interne devono essere indentate di quattro spazi.

```cpp
if (sound_action[id] == 2)
{
    ++report.defaulted_sounds;
}
else
{
    ++report.cleared_sounds;
}
```
