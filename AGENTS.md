# Regola operativa obbligatoria

In questo repository Codex NON deve mai modificare la codebase senza esplicita autorizzazione dell'utente.

Per "modificare la codebase" si intende qualsiasi operazione che crea, modifica, sposta, elimina o formatta file del progetto, inclusi ma non limitati a:
- file sorgente
- configurazioni
- build script
- test
- documentazione tecnica nel repository
- file generati o metadati del progetto

Sono consentite senza autorizzazione solo operazioni di sola lettura, come:
- ispezionare file
- eseguire `git status`, `git diff`, `git log`
- cercare testo con `rg`
- analizzare il codice
- proporre patch o piani di modifica senza applicarli

Prima di ogni modifica, Codex deve:
1. descrivere esattamente quali file intende modificare;
2. spiegare perche' la modifica e' necessaria;
3. attendere autorizzazione esplicita dell'utente.

Frasi come "procedi", "applica", "modifica", "implementa", "correggi nel codice" o equivalenti costituiscono autorizzazione solo se riferite chiaramente alla modifica proposta.

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
