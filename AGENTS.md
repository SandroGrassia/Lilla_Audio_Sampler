# Modifica della codebase

In questo repository Codex può modificare la codebase con esplicita autorizzazione. Assieme alla modifica del codice è anche autorizzata la compilazione.


# Commit, Merge e operazioni verso GitHub

In questo repository Codex può effettuare le operazioni GIT locali e su GitHub quando richieste.

# Recap per Commit

Anche senza indicazione esplicita, si devono intendere sempre in lingua Inglese.

# Aggiornamento release

L'aggiornamento della release del codice deve comprendere l'aggiornamento del nome release nel file config.h, l'aggiornamento del file binario .hex col nome della release, l'aggiornamento di README.md, della User Guide.md/.pdf, infine l'aggiornamento di Releases su GitHub, con relativa descrizione delle modifiche più rilevanti per l'utilizzatore (queste generalmente le indico io)


# Push della release del codice su origin 

Il push su origin/main del codice, che rappresenta la pubblicazione dello stato dell'arte del progetto e condivisione dei file per l'utilizzatore finale tramite la pagina principale in GitHub, va effettuato conservando tutta la catena dei commit.


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

# Formattazioni speciali

Si devono mantenere in evidenza i blocchi contenuti all'interno di AudioNoInterrupts() e AudioInterrupts() tenendo una riga vuota sopra e sotto, tranne casi in cui ci sono graffe che delimitano, ad esempio:
{
    AudioNoInterrupts();
    Players_Manager.Update_all_Preset_volume(Patch_id, Volume_float[volume_patch]);
    Players_Manager.Broadcast_volume();
    AudioInterrupts();
}


# Popup

Per i popup si devono possibilmente utilizzare le funzioni Show_popup_text (per la scrittura del testo) e Confirm_frame_on_RED (per effettuare una selezione)


# Merge

Ogni volta che si esegue il merge del branch attuale nel main, occorre conservare tutti i Commit intermedi e garantire la visualizzazione del ramo branch e del ramo main.


# Dichiarazioni e Definizioni

Dichiarazioni e definizioni dello di un oggetto/variabile/costante/funzione vanno sempre inserite nella stessa coppia abcd.h/abcd.cpp.
Si deve evitare di inserire una dichiarazione in un file abcd.h e la corrispondente definizione in efgh.cpp


# Controlli di sicurezza nelle funzioni

Per assicurare la massima velocità di esecuzione, se i chiamanti NON violano mai i range ammissibili, le funzioni NON devono includere il controllo dei valori ammissibili.