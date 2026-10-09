<a id="lilla-user-guide"></a>

# Lilla Manuale Utente

[English](User%20Guide.md) | [Italiano](Manuale%20Utente.md) | [Deutsch](Benutzerhandbuch.md) | [Français](Guide%20utilisateur.md)

Per **LILLA Audio Sampler 2026 | PCB2026_R1 | firmware 7.0.2**

Edizione del manuale: **8 ottobre 2026**

Edizione stampabile in inglese: [Lilla User Guide PDF](User%20Guide.pdf).

<img src="doc/assets/images/0.jpg" alt="Schermata di avvio di LILLA con versione del firmware e informazioni sulla memoria" width="37%">

*Schermata di benvenuto. Le fotografie del manuale mostrano uno strumento funzionante; numeri delle patch, nomi dei file e valori sono esempi.*

LILLA riunisce riproduzione di campioni, registrazione dell'ingresso linea, campionamento live e loop MIDI in un solo strumento. Questo manuale accompagna dalla prima patch suonabile alla creazione dei propri suoni e alla conservazione di una sessione completa.

Per suonare subito, iniziare da **Primi passi**. Prima di costruire una libreria, leggere **File, suoni e patch**: capire cosa conserva ogni operazione di salvataggio rende più semplice l'uso dello strumento.

**Come leggere il manuale:** i nomi in grassetto, come **Select**, indicano i controlli fisici; il testo `UPPERCASE` indica etichette o messaggi sul display. **Tools > Setup** significa impostare il selettore Tools su Setup e poi premere Tools. Le procedure numerate indicano l'ordine delle operazioni. Le fotografie illustrano l'aspetto delle pagine; le istruzioni descrivono il firmware del repository, comprese le modifiche successive alle fotografie.

Le procedure sono state confrontate con il firmware e con le fotografie del display fornite. Resta da effettuare una verifica completa sullo strumento fisico.

<a id="contents"></a>

## Indice

- [Primi passi](#getting-started)
- [Connessioni e pulsanti](#io-connections-and-buttons)
- [Controlli e navigazione](#controls-and-navigation)
- [File, suoni e patch](#files-sounds-and-patches)
- [Performance](#performance)
- [Modificare un suono](#editing-a-sound)
- [Importare audio](#importing-audio)
- [Registrare con Sampler](#recording-with-sampler)
- [Live Sampler](#live-sampler)
- [MIDI Loop](#midi-loop)
- [Mixer, Delay e filtri](#mixer-delay-and-filters)
- [Setup e controlli MIDI](#setup-and-midi-controls)
- [Backup e ripristino](#backup-and-restore)
- [Aggiornare il firmware su Windows](#updating-the-firmware-on-windows)
- [Aggiornare il firmware su Mac](#updating-the-firmware-on-mac)
- [Risoluzione dei problemi](#troubleshooting)
- [Progetti pratici](#practical-projects)
- [Riferimento rapido](#quick-reference)
- [Glossario](#glossary)

**Procedure frequenti:** [Creare uno split di tastiera](#build-a-keyboard-split) | [Accordare un suono e usare Auto Tune](#tune-a-sound-and-use-auto-tune) | [Catturare un loop live in una patch](#capture-a-live-loop-into-a-patch) | [Preparare un archivio completo](#plan-a-complete-archive)

<a id="getting-started"></a>

## Primi passi

LILLA è un campionatore hardware polifonico e multitimbrico con un massimo di 16 voci di riproduzione. Una patch Performance può contenere fino a otto suoni, ciascuno con il proprio canale MIDI e intervallo di tastiera. È possibile riprodurre audio importato, registrare l'ingresso linea, lavorare con un buffer live temporaneo e registrare loop MIDI a quattro tracce.

<a id="choose-the-right-mode"></a>

### Scegliere la modalità adatta

| Per... | Scegliere | Materiale utilizzato |
| --- | --- | --- |
| Suonare uno split di tastiera, un layer o una configurazione multitimbrica | **Performance** | Una patch con un massimo di otto slot sonori. |
| Registrare una take, conservarla nella memoria delle registrazioni o esportare un WAV | **Sampler** | Una registrazione audio mono o stereo in Flash. |
| Esplorare l'audio in ingresso e catturare un frammento selezionato | **Live Sampler** | Un buffer audio circolare temporaneo. |
| Registrare e riprodurre frasi suonate sul controller MIDI | **MIDI Loop** | Quattro tracce di eventi MIDI che utilizzano la patch corrente. |

Usare Sampler per una registrazione da ascoltare, convertire o esportare. Usare Live Sampler per trovare il suono desiderato all'interno di un flusso audio continuo.

<a id="connect-and-play"></a>

### Collegare e suonare

1. Collegare un controller MIDI all'ingresso MIDI di LILLA.
2. Collegare l'uscita linea stereo al mixer, all'amplificatore o all'interfaccia audio. Iniziare con livelli di ascolto bassi.
3. Accendere LILLA e attendere il completamento dell'avvio.
4. Impostare il selettore Modes su **Performance**.
5. Ruotare **Select** per evidenziare il numero della patch, quindi ruotare **Value** per scegliere una patch esistente.
6. Impostare il controller sul canale MIDI indicato per un suono della patch.
7. Suonare note comprese nell'intervallo `FROM K` e `TO K` di quel suono.
8. Aumentare gradualmente **Line Out Vol**.

**Cosa appare:** la pagina Performance mostra un numero di patch e le righe dei suoni attivi. Il canale MIDI e l'intervallo di tastiera di ciascuna riga determinano quali note attivano il suono. Una patch con otto slot non deve necessariamente usarli tutti.

**Cosa si sente:** un suono quando si esegue una nota nell'intervallo di una riga attiva sul canale MIDI assegnato. Se non si sente nulla, controllare prima canale e intervallo, poi eventualmente il campione o l'inviluppo.

<a id="your-first-edit"></a>

### La prima modifica

1. Premere **S1** se il suono 1 è attivo, oppure il pulsante di un altro suono attivo.
2. Ruotare Select per evidenziare `GAIN`, quindi ruotare leggermente Value.
3. Suonare alcune note e ascoltare la variazione.
4. Selezionare `RETURN` e premere Select.
5. In Performance, scegliere `SAVE` quando disponibile.

Questo introduce il normale ciclo di modifica: **aprire un suono → modificarlo → tornare a Performance → salvare la patch**. Tornare da una pagina conserva le modifiche di lavoro, ma non sostituisce il salvataggio.

<a id="finish-a-session"></a>

### Terminare una sessione

Prima di spegnere, salvare le patch modificate e i loop MIDI da conservare, e completare i salvataggi delle catture Live Sampler ancora in sospeso. Attendere la fine delle scritture o delle esportazioni. Il buffer live non sopravvive allo spegnimento.

Se non è caricato audio adatto, seguire [Importare audio](#importing-audio). L'importazione sostituisce la libreria audio in Flash e cancella le registrazioni esistenti: effettuare prima un backup.

<a id="io-connections-and-buttons"></a>

## Connessioni e pulsanti

| Connessione o pulsante | Connettore | Descrizione |
| --- | --- | --- |
| Line in | Jack da 3,5 mm | Ingresso linea stereo / ingresso stereo per microfono dinamico. |
| Line out | Jack da 3,5 mm | Uscita linea stereo, 3,1 Vpp. |
| Phones line | Jack da 3,5 mm | Uscita cuffie principale. |
| Phones pre-listen | Jack da 3,5 mm | Uscita cuffie di preascolto. |
| MIDI IN | Jack da 3,5 mm | Ingresso MIDI. |
| MIDI OUT | Jack da 3,5 mm | Uscita MIDI. |
| Gate IN | Jack da 3,5 mm | Ingresso Gate, +5 V. |
| Gate OUT | Jack da 3,5 mm | Uscita Gate, +5 V. |
| USB-C | USB-C | Alimentazione +5 V CC e programmazione. |
| Pulsante Firmware_upload mode | Pulsante | Accesso alla modalità di caricamento del firmware. |
| Pulsante On/off | Pulsante | Accensione o spegnimento dello strumento. |

**Sviluppi futuri:** MIDI OUT, Gate IN e Gate OUT sono fisicamente disponibili e accessibili tramite classi già presenti nel firmware. Attualmente nessuna funzione utilizzabile dall'utente impiega queste connessioni; sono disponibili per sviluppi futuri.

<a id="controls-and-navigation"></a>

## Controlli e navigazione

<img src="doc/assets/images/top.jpg" alt="Pannello superiore di LILLA con display, encoder, selettori di modalità e pulsanti dei suoni" width="100%">

*Panoramica del pannello superiore: controlli fisici e loro posizione.*

Il riquadro bianco di selezione identifica il campo o il comando che risponde ai controlli di navigazione. Nelle fotografie fornite le etichette sono generalmente ciano, i valori modificabili e i comandi gialli, i titoli delle pagine rossi.

**Ruotare e premere un encoder sono azioni distinte.** Per esempio, ruotare Value modifica il parametro evidenziato, mentre premerlo può azzerarlo o commutarlo in alcune pagine.

Gli stessi controlli svolgono funzioni diverse secondo la pagina attiva. Seguire il campo evidenziato e le opzioni attualmente visibili sul display.

| Controllo | Uso principale |
| --- | --- |
| Selettore Modes | Scelta tra Sampler, Live Sampler, Performance e MIDI Loop. |
| Selettore Tools | Scelta tra Mixer, Delay, Setup e Test. |
| Pulsante Tools | Apre lo strumento selezionato; il LED Tools segnala l'accesso agli strumenti. |
| Select, rotazione | Sposta la selezione tra campi e voci di menu. |
| Select, pressione | Esegue un comando, conferma una scelta o entra/esce da un gruppo di campi. |
| Value, rotazione | Modifica il parametro evidenziato. |
| S1-S8 | Apre i suoni attivi in Performance; in Sampler S1 apre una registrazione mono o il canale sinistro e S2 il destro; in Live Sampler cattura negli slot sonori. |
| From / To | Regola i limiti del campione in Sound Edit e la regione di riproduzione live in Live Sampler. |
| Step | Modifica l'incremento di editing; la funzione della pressione dipende dalla pagina. |
| Line Out Vol | Regola il volume della patch nelle pagine legate alla Performance. |
| Pre Listen Vol | Regola il livello di preascolto. |
| Resolution / Downsampling | Modifica il carattere della riproduzione tramite riduzione dei bit e ripetizione dei campioni. |
| Cutoff | Regola il filtro passa-basso comune; la pressione ripristina la frequenza di taglio massima. |
| Tuning Tone | Attiva il tono di riferimento per l'accordatura; quando attivo, la rotazione ne regola il livello. |
| Loop / Tempo | Seleziona e controlla i loop MIDI e la loro temporizzazione. |
| Track 1-4 / Rec 1-4 | Controlla le singole tracce dei loop MIDI e la loro registrazione. |

Per aprire uno strumento, impostare il selettore Tools sulla posizione desiderata e premere **Tools**. Test apre il monitor MIDI. Per tornare, usare Tools dove previsto, oppure selezionare la modalità operativa desiderata.

Nei dialoghi di conferma, ruotare **Select** per scegliere un'opzione e premerlo per confermare. Leggere il dialogo prima di confermare: salvare, scartare, cancellare e ripristinare hanno conseguenze diverse.

I menu sono dinamici. Un comando può essere nascosto quando lo stato corrente non ne consente l'uso, per esempio se non esiste una registrazione da esportare. Una patch salvata e non modificata può mostrare meno comandi di una patch con modifiche in sospeso.

<a id="three-navigation-patterns"></a>

### Tre modalità di navigazione

**Comandi di menu:** ruotare Select finché il riquadro circonda il comando, quindi premere Select. Se appare una conferma, scegliere la risposta desiderata e premere nuovamente Select.

**Modifica dei parametri:** ruotare Select per evidenziare il parametro, poi ruotare Value. Ascoltare durante la regolazione: molte modifiche sono immediatamente udibili.

**Tabelle di suoni o sorgenti:** selezionare prima la riga o la sorgente, poi premere Select per entrare nei campi modificabili. Premere nuovamente Select per uscire dal gruppo, nelle pagine che prevedono questo comportamento.

<a id="useful-shortcuts"></a>

### Scorciatoie utili

| Pagina | Azione | Risultato |
| --- | --- | --- |
| Performance | Premere un pulsante S1-S8 attivo | Apre l'editor del suono corrispondente. |
| Sound Edit da Performance | Premere nuovamente lo stesso pulsante del suono | Apre la sua pagina VCF. |
| Sound Edit da Sampler | Premere S1, oppure S2 per lo stereo | Seleziona il canale della registrazione; lo stesso pulsante mantiene quel canale in Sound Edit. |
| Sound Edit, `PITCH` selezionato | Premere Value | Azzera la regolazione dell'intonazione. |
| Sound Edit, `PITCH` selezionato | Premere Select | Esegue Auto Tune sulla regione audio selezionata. |
| Sound Edit | Premere From | Porta l'inizio della regione all'inizio della sorgente. |
| Sound Edit | Premere To | Commuta tra modifica del limite e modifica della porzione. |
| Sound Edit | Premere Step | Seleziona l'incremento di ritaglio relativo alla regione. |
| Performance o Sound Edit, `PAN` selezionato | Premere Value | Centra il pan. |
| Live Sampler | Premere Step | Commuta il comportamento di blocco del punto iniziale. |
| Controlli comuni di riproduzione | Premere Cutoff | Ripristina il taglio massimo del filtro passa-basso comune. |
| Controlli comuni di riproduzione | Premere Line Out Vol | Ferma i player attivi; azzera anche il feedback del delay al di fuori del contesto Delay di Direct Sampler. |

Usare l'ultima scorciatoia per fermare rapidamente le note in riproduzione. In MIDI Loop ferma anche la riproduzione delle tracce.

<a id="files-sounds-and-patches"></a>

## File, suoni e patch

| Termine | Significato |
| --- | --- |
| File audio | Il campione sorgente usato per la riproduzione. L'audio importato viene convertito in audio RAW mono in Flash. |
| Registrazione | Audio registrato da Sampler nella sua area Flash; può essere mono o stereo. |
| Suono | Un file sorgente insieme alle impostazioni di riproduzione, come ritaglio, intonazione, inviluppo, pan e guadagno. |
| Strumento / slot sonoro | Una delle otto posizioni disponibili in una patch, con mappatura MIDI, nota di riferimento, intervallo di tastiera e impostazioni del filtro. |
| Patch | L'insieme dei suoni e delle impostazioni usati per una performance. |
| Loop MIDI | Eventi MIDI registrati e organizzati in quattro tracce. Non contiene i campioni audio riprodotti da tali eventi. |
| Buffer live | Audio temporaneo in PSRAM per Live Sampler. |

Il firmware può memorizzare fino a 200 patch e 800 record di suoni. Si tratta di capacità di archiviazione; lo strumento dispone di un massimo di 16 voci simultanee. La polifonia disponibile dipende anche dal carico della riproduzione.

La Flash contiene la libreria audio importata e le registrazioni Sampler. La PSRAM contiene l'audio live. La scheda microSD serve per importazione, esportazione WAV, backup e file di loop MIDI.

<a id="follow-the-sound-from-source-to-keyboard"></a>

### Dalla sorgente audio alla tastiera

Una configurazione tipica ha tre livelli:

**File audio → Impostazioni del suono → Strumento in una patch**

Per esempio, `piano.wav` viene importato come `piano.raw`. Un suono seleziona quella sorgente e ne definisce ritaglio, inviluppo e accordatura. Uno slot dello strumento assegna nota di riferimento, canale MIDI e intervallo di tastiera suonabile.

Modificare il ritaglio regola quale parte della sorgente viene riprodotta, senza tagliare il file originale. Rimuovere uno strumento ne elimina la posizione nella patch, senza cancellare l'audio sorgente.

Un'etichetta come `SOUND 1` indica il primo slot della patch corrente. Non corrisponde al file audio 1, alla registrazione 1 o alla patch 1.

<a id="what-survives-power-off"></a>

### Cosa rimane dopo lo spegnimento?

| Materiale | Come conservarlo |
| --- | --- |
| Modifiche a una patch e ai suoi suoni | Salvare da Performance. |
| Una registrazione Sampler completata | Terminare la registrazione con `STOP`; usare backup o esportazione WAV per una copia esterna. |
| Limiti A/B delle registrazioni Sampler | Salvati automaticamente durante l'editing; richiamati con la registrazione per la tastiera e l'export RAW/WAV. Gli altri parametri di riproduzione restano nella sessione corrente. |
| Una registrazione Sampler convertita con `MAKE_RAW` | Conservare la sorgente Flash generata e salvare la patch che la utilizza. |
| Audio ancora nel buffer Live Sampler | Catturare la regione desiderata in uno slot e salvare la patch risultante. |
| Un suono appena catturato con Live Sampler | Salvare la patch affinché l'audio in sospeso venga scritto in Flash. |
| Un loop MIDI | Salvarlo su microSD da MIDI Loop. |
| Una copia trasportabile del proprio lavoro | Conservare configurazione, audio delle registrazioni, libreria sorgente e file di loop MIDI come descritto in Backup e ripristino. |

<a id="patch-numbers-and-the-temporary-session"></a>

### Numeri delle patch e area di lavoro temporanea

Le patch normali usano gli **ID 0-199**. La **patch 200** è l'area di lavoro temporanea per il campionamento, non uno slot normale aggiuntivo.

La prima cattura Live Sampler riuscita crea una patch normale usando il **primo ID disponibile tra 0 e 199**. Le catture successive possono riempire gli slot rimanenti. Passare a Performance per controllare e salvare la patch.

<a id="file-names-matter"></a>

### L'importanza dei nomi dei file

L'intestazione del suono mostra solo i primi otto caratteri del nome base della sorgente, senza `.raw`. Due file con nomi simili possono quindi apparire uguali. Scegliere inizi brevi e distintivi, come `BassDry` e `BassFX`.

LILLA mantiene l'identità di un file anche quando manca l'audio. Reimportando lo stesso nome base si possono ricollegare i suoni che lo usano. Rinominare un file crea un'identità diversa: considerare i nomi della libreria parte del progetto.

La tabella dei nomi supporta 260 identità RAW, comprese la sorgente di riserva, i file generati e i riferimenti conservati ai file mancanti. Memoria audio libera e identità di file disponibili sono risorse distinte.

<a id="performance"></a>

## Performance

<img src="doc/assets/images/1.jpg" alt="Pagina Performance con una mappatura di tastiera a sette suoni" width="37%">

*Una patch di esempio distribuita su sette slot sonori. Ogni riga ha nota di riferimento, intervallo e guadagno propri.*

Performance è la pagina principale per costruire e salvare uno strumento suonabile. Si legge dall'alto verso il basso: patch e volume in alto, controlli comuni del carattere sonoro al centro, righe degli strumenti attivi in basso.

<a id="choose-a-patch"></a>

### Scegliere una patch

Evidenziare il numero della patch con **Select** e ruotare **Value** per scorrere le patch esistenti.

Se la patch corrente contiene modifiche, LILLA chiede come procedere prima di cambiarla. Si può restare, scartare le modifiche oppure salvarle e proseguire. Usare le scelte visualizzate e controllare il messaggio prima di cambiare pagina.

<a id="map-sounds-to-your-controller"></a>

### Associare i suoni al controller

1. Evidenziare una riga di suono.
2. Premere **Select** per entrare nei suoi campi.
3. Ruotare **Select** per spostarsi tra i campi.
4. Ruotare **Value** per modificare il campo selezionato.
5. Premere **Select** per uscire dai campi della riga.

| Campo | Funzione |
| --- | --- |
| `SOUND` | Slot sonoro nella patch, numerato da 1 a 8. |
| `LOCK` | Protegge il suono da alcuni controlli di performance, inclusi pitch bend, resolution e downsampling. Influisce anche sulla gestione del rilascio delle note. |
| `P` | Precedenza di riproduzione: dà priorità al suono nell'assegnazione delle voci. Non garantisce voci illimitate. |
| `MIDI` | Canale MIDI di ricezione, visualizzato da 1 a 16. |
| `ROOT K` | Nota di riferimento usata per la mappatura dell'intonazione del suono. |
| `FROM K` / `TO K` | Intervallo inclusivo di tastiera che attiva questo suono. |
| `PAN` | Posizione stereo; premere Value su questo campo per centrarla. |
| `GAIN` | Livello del suono. |

Per uno split di tastiera, assegnare due suoni allo stesso canale MIDI con intervalli separati. Per un layer, usare intervalli sovrapposti. Per la riproduzione multitimbrica, assegnare canali MIDI diversi.

<a id="build-a-keyboard-split"></a>

### Creare uno split di tastiera

Uno split permette di suonare un basso su una parte della tastiera e un pad, un pianoforte o un lead sull'altra.

1. Iniziare con una patch contenente due suoni attivi. Se necessario, aprire un suono esistente e usare `CLONE` per aggiungere uno slot.
2. Scegliere sorgente e inviluppo adatti per ogni suono.
3. In Performance, assegnare lo stesso canale MIDI ai due slot.
4. Impostare `TO K` del suono 1 sull'ultima nota della zona inferiore.
5. Impostare `FROM K` del suono 2 sulla nota immediatamente successiva.
6. Provare le due note ai lati del punto di divisione.
7. Bilanciare i guadagni e salvare la patch.

Gli intervalli sono inclusivi. Se il suono 1 termina sulla stessa nota da cui inizia il suono 2, quella nota attiva entrambi.

<a id="build-a-layer-or-multitimbral-setup"></a>

### Creare un layer o una configurazione multitimbrica

Per un **layer**, assegnare lo stesso canale MIDI e intervalli sovrapposti a due o più slot. Iniziare con guadagni individuali bassi, poi aumentarli ascoltando il risultato combinato.

Per una **configurazione multitimbrica**, assegnare canali diversi ai vari slot. Un sequencer o controller può così indirizzare separatamente ogni parte. Il canale MIDI determina quale strumento risponde; il numero dello slot non assegna automaticamente quel canale.

Ogni layer attivato utilizza voci di riproduzione. Un accordo di quattro note con due layer può richiedere otto voci, senza contare le code di rilascio.

<a id="set-the-root-key"></a>

### Impostare la nota di riferimento

La nota di riferimento è il riferimento di tastiera del campione. Su quella nota, il campione viene riprodotto con la propria regolazione di intonazione; le note superiori e inferiori traspongono la sorgente rispetto a quel riferimento.

Per un campione intonato, impostare la nota di riferimento sulla nota rappresentata dalla registrazione. Per un colpo assegnato a un solo tasto, impostare `FROM K` e `TO K` sulla stessa nota di attivazione e scegliere una nota di riferimento adatta all'intonazione desiderata.

I nomi delle ottave dipendono da `FIRST OCTAVE` in Setup. Confrontando le impostazioni con un altro dispositivo, verificare la nota MIDI effettiva oltre al nome dell'ottava visualizzato.

<a id="save-or-duplicate-a-patch"></a>

### Salvare o duplicare una patch

Usare i comandi visualizzati nel menu Performance:

- `SAVE`: memorizza la patch corrente e i suoni modificati.
- `CLONE`: crea una copia in un altro slot patch disponibile.
- `SAVE_AS_NEW`: memorizza il risultato modificato come nuova patch.
- `EXIT`: scarta le modifiche correnti tramite la procedura di uscita di Performance.
- `DROP`: elimina la patch dopo la conferma di cancellazione.

Il comando `RETURN` di un suono mantiene le modifiche nella sessione corrente. Salvare la patch per renderle permanenti.

Usare `CLONE` per creare una seconda patch da sviluppare a partire da una esistente. Usare `SAVE_AS_NEW` dopo aver modificato una patch per conservare il risultato con un altro ID disponibile. Leggere destinazione e conferma prima di procedere.

**Prima di passare al campionamento, salvare la patch in modifica.** Live Sampler può mantenere in memoria la precedente patch Performance, ma la creazione di una nuova patch catturata richiede che quella precedente non abbia modifiche in sospeso.

<a id="editing-a-sound"></a>

## Modificare un suono

<img src="doc/assets/images/2.jpg" alt="Pagina Sound Edit con inviluppo, modalità di riproduzione e forma d'onda del campione" width="37%">

*La forma d'onda appartiene alla sorgente selezionata. `FROM`, `TO` e `TOT` descrivono la regione suonabile; `TRIM STEP` controlla l'incremento di modifica.*

Da Performance, premere **S1-S8** per un suono attivo per aprire Sound Edit. Uno slot inutilizzato mostra `SOUND ... IS NOT USED`; premerlo non crea un nuovo suono.

<a id="choose-and-trim-the-source"></a>

### Scegliere e ritagliare la sorgente

1. Evidenziare il campo del file con **Select**.
2. Ruotare **Value** per scegliere una sorgente disponibile.
3. Suonare dal controller MIDI.
4. Ruotare **From** e **To** per regolare la regione di riproduzione.
5. Ruotare **Step** per scegliere un incremento adatto: usare passi grandi per individuare la regione, poi passi piccoli per rifinirla.

Cambiare il file sorgente riporta la regione all'intera lunghezza del nuovo file e azzera la regolazione dell'intonazione. Ricontrollare i limiti dopo il cambio di file.

<a id="work-from-a-rough-cut-to-a-precise-region"></a>

### Dal taglio approssimativo alla regione precisa

Iniziare con un incremento ampio per eliminare silenzi lunghi o individuare una frase. Ridurlo vicino all'attacco e al punto finale desiderati. Riprodurre più volte il campione durante la rifinitura: la forma d'onda aiuta a trovare un evento, ma l'ascolto rivela se è stato tagliato l'attacco o lasciata una coda indesiderata.

Premere **From** riporta l'inizio all'inizio della sorgente. Premere **To** commuta tra modifica di un estremo e modifica di una porzione. Nella seconda modalità, spostare From può muovere la regione mantenendone la lunghezza; osservare il display durante lo spostamento.

Una regione corta è utile per una texture ripetuta. Una regione più lunga può preservare attacco e decadimento naturali di uno strumento. Se si cambia successivamente sorgente, ripetere il ritaglio perché il cambio ne reimposta i limiti.

<a id="shape-playback"></a>

### Modellare la riproduzione

Usare Select per evidenziare un parametro e Value per modificarlo:

| Parametro | Funzione |
| --- | --- |
| Pitch | Accorda il campione. |
| Gain / Pan | Regola livello e posizione stereo. |
| Attack | Imposta la rapidità con cui il suono raggiunge il livello iniziale. |
| Decay | Imposta la transizione verso il livello di sustain. |
| Sustain | Imposta il livello mantenuto dall'inviluppo. |
| Release | Imposta la dissolvenza dopo il rilascio. |
| Play mode | Sceglie direzione di riproduzione e comportamento singolo o in loop. |
| Noclick | Regola lo smussamento dei limiti del loop, dove disponibile. |

| Modalità di riproduzione | Lettura della regione | Uso tipico |
| --- | --- | --- |
| Once FWD | Dall'inizio alla fine, una sola volta. | Colpi, parole e decadimenti naturali. |
| Once REV | Dalla fine all'inizio, una sola volta. | Impatti e crescendi al contrario. |
| Loop FWD | Ripete in avanti. | Toni sostenuti e frasi ripetute. |
| Loop FWD/REV | Alterna la direzione, iniziando in avanti. | Texture che invertono la direzione ai limiti. |
| Loop REV/FWD | Alterna la direzione, iniziando all'indietro. | Variante di loop alternato con partenza inversa. |
| Loop REV | Ripete all'indietro. | Texture ripetute al contrario. |

Inviluppo e rilascio delle note influiscono comunque sull'ascolto. Una regione in loop può sfumare con l'inviluppo; una sorgente a riproduzione singola ha comunque una fine.

**Un inviluppo semplice da cui partire:** usare un attacco breve per le percussioni, più lento per un pad, e un rilascio abbastanza lungo da evitare un'interruzione brusca. Con un loop, alzare il sustain per ascoltare chiaramente la regione ripetuta prima di modellare decay e release.

Per un loop pulito, rifinire inizio e fine prima di aumentare Noclick. L'intervallo disponibile per Noclick dipende dalla regione selezionata.

<a id="tune-a-sound-and-use-auto-tune"></a>

### Accordare un suono e usare Auto Tune

Il valore `PITCH` indica il rapporto di riproduzione: **1.000** corrisponde alla velocità originale. Aumentare l'intonazione accelera anche la normale riproduzione del campione; diminuirla la rallenta.

1. Selezionare `PITCH`.
2. Ruotare **Value** per accordare a orecchio, oppure premere **Value** per azzerare la regolazione.
3. Premere **Select** per eseguire **Auto Tune** sulla regione scelta.
4. Ascoltare il risultato sulla nota di riferimento e confrontarlo con gli altri strumenti.
5. Salvare la patch per conservare la regolazione.

Auto Tune è disponibile sia in riproduzione singola sia in loop. Analizza la componente di frequenza più forte e la avvicina alla nota cromatica più vicina. Un'armonica dominante, un attacco rumoroso o un campione non intonato possono non corrispondere alla fondamentale musicale attesa. Per un risultato più chiaro, selezionare una porzione stabile e intonata e verificare a orecchio.

Se appare un errore `AUTO-TUNE`, leggerne la causa: regione non valida, sorgente indisponibile, assenza di segnale misurabile o operazione audio occupata richiedono interventi diversi. Auto Tune non sostituisce la scelta della nota di riferimento e dell'intervallo di tastiera corretti.

`MAX PITCH` indica il limite superiore di riproduzione disponibile per il percorso della sorgente corrente. Può cambiare con la cache e la preparazione della riproduzione; non tutte le sorgenti hanno necessariamente lo stesso limite di trasposizione.

<a id="return-clone-or-remove-a-sound"></a>

### Tornare, clonare o rimuovere un suono

I comandi seguenti riguardano i suoni aperti da Performance. Per una registrazione aperta da Sampler, l'unico comando di menu è `RETURN`; vedere [Modificare una registrazione prima della conversione o dell'esportazione](#edit-a-recording-before-conversion-or-export).

- `RETURN` mantiene le modifiche e torna alla precedente pagina di performance.
- `CLONE` copia lo strumento in uno slot libero della patch corrente. Regolare intervallo, nota di riferimento o sorgente della copia secondo necessità.
- `DROP` rimuove lo strumento dalla patch corrente.

Al termine, salvare la patch. Rimuovere uno slot sonoro è diverso da cancellare il suo file audio sorgente.

<a id="importing-audio"></a>

## Importare audio

> **L'importazione sostituisce la libreria audio.** Confermarla cancella i precedenti file audio in Flash e le registrazioni Sampler. Prima di procedere, creare un backup e conservare l'audio sorgente sul computer.

<img src="doc/assets/images/12.jpg" alt="Pagina di importazione audio con file sorgente, capacità Flash e avviso di cancellazione" width="37%">

*Prima di importare, controllare sia il rapporto sulle sorgenti sia la capacità di destinazione. La fotografia indica 35 secondi; il limite del firmware attuale è 3 MiB di PCM mono decodificato, circa 35,7 secondi.*

<a id="prepare-the-microsd-card"></a>

### Preparare la scheda microSD

Creare `/LILLA_AUDIO` nella radice della scheda e inserirvi direttamente i file audio.

| Formato | Requisiti di importazione |
| --- | --- |
| `.raw` | PCM mono senza intestazione, 16 bit con segno little-endian, a 44,1 kHz. |
| `.wav` | PCM non compresso, 16 bit, 44,1 kHz, mono o stereo. |
| `.aif` / `.aiff` | AIFF non compresso, 16 bit, 44,1 kHz, mono o stereo. |
| `.mp3` | Mono o stereo alle frequenze MP3 standard da 8 a 48 kHz; convertito a 44,1 kHz. |

I file stereo vengono mediati in mono. Per conservare una sorgente stereo come due file suonabili indipendentemente, preparare prima dell'importazione file sinistro e destro separati con nomi diversi.

L'audio importato viene memorizzato come `<basename>.raw`. Usare nomi base distinti: `piano.wav` e `piano.mp3` producono entrambi `piano.raw` e sono trattati come duplicati.

Una struttura semplice della scheda è:

```text
Radice microSD/
  LILLA_AUDIO/
    BassDry.wav
    Bell.aiff
    DrumLoop.mp3
    Texture.raw
```

Inserire i file direttamente in quella cartella. Usare nomi base distinti, lunghi al massimo 31 byte. I nomi ASCII semplici facilitano il rispetto del limite. Evitare nomi riservati alle registrazioni, come `P12.raw`.

Un MP3 compresso può occupare poco sul computer ma molto di più dopo la conversione. Valutare la memoria Flash necessaria dal rapporto di importazione, che considera l'audio decodificato.

Ogni file importato è limitato a 3 MiB di PCM mono decodificato, circa 35,7 secondi. I file più lunghi vengono troncati. Decodifica MP3 e conversione della frequenza possono richiedere più tempo dell'importazione PCM.

<a id="import-the-files"></a>

### Importare i file

1. Inserire la scheda microSD preparata.
2. Aprire **Tools > Setup**.
3. Selezionare `IMPORT AUDIO FILES FROM /LILLA_AUDIO` e premere Select.
4. Controllare il rapporto di importazione e l'avviso finale di cancellazione.
5. Confermare solo quando si è pronti a sostituire libreria e registrazioni correnti.
6. Attendere il completamento di copia, configurazione della memoria e riavvio.
7. Aprire Sound Edit e selezionare la sorgente importata da utilizzare.

Dopo l'importazione, ascoltare alcune sorgenti in Sound Edit prima di ricostruire una patch intera. Controllare attacco, punto finale e intonazione, soprattutto per sorgenti lunghe o convertite.

**Per uno strumento stereo:** esportare sinistra e destra come file mono separati, importarli entrambi, assegnarli a due slot con nota di riferimento e intervalli uguali, quindi posizionarli a sinistra e a destra con il pan. L'importazione standard di un file stereo produce una sorgente mono.

La schermata di importazione segnala file non validi, duplicati e problemi di capacità Flash. Se resta poca memoria per campionare, preparare una libreria più piccola o file più corti.

<a id="recording-with-sampler"></a>

## Registrare con Sampler

Sampler registra l'ingresso linea in Flash e permette di ascoltare, convertire o esportare il risultato.

<img src="doc/assets/images/6.jpg" alt="Sampler in PAUSE+REC con indicatori dei livelli sinistro e destro" width="37%">

*Gli indicatori consentono di regolare il guadagno prima di registrare. Tempo di registrazione libero e spazio disponibile per i file audio sono mostrati separatamente.*

<a id="understand-the-recording-stages"></a>

### Comprendere le fasi della registrazione

**Monitorare, registrare, fermare, ascoltare, poi convertire o esportare.**

Il monitoraggio in `PAUSE+REC` permette di preparare sorgente e livelli. `MONO_REC` o `STEREO_REC` avvia la registrazione; `STOP` la termina. Poi `MAKE_RAW` crea una sorgente per una patch, mentre `EXPORT_WAV_TO_SD` crea un file da usare fuori da LILLA.

All'avvio di una take vengono registrati 20 ms di audio prima di accettare il comando successivo, per completare il fade-in iniziale. Uno Stop immediato viene elaborato dopo questa breve pausa e conserva la take. Fermare o lasciare Sampler attende la conclusione della registrazione prima di salvarla.

Una registrazione Flash può essere suonata da tastiera e modificata prima della conversione. Una conversione RAW o un export WAV riuscito rimuove la registrazione sorgente e ne libera lo spazio. Effettuare prima un backup se si vuole conservare l'intera take originale.

<a id="make-a-recording"></a>

### Effettuare una registrazione

1. Collegare la sorgente audio all'ingresso linea stereo.
2. Impostare il selettore Modes su **Sampler**.
3. Selezionare `PAUSE+REC` per monitorare l'ingresso prima della registrazione.
4. Regolare con Value il guadagno d'ingresso visualizzato quando è selezionato il relativo campo. Osservare entrambi gli indicatori ed evitare picchi rossi persistenti.
5. Selezionare `MONO_REC` o `STEREO_REC`.
6. Avviare la sorgente e osservare tempo trascorso e memoria disponibile.
7. Al termine selezionare `STOP`.
8. Selezionare la registrazione da ascoltare e regolarne il volume di riproduzione.

**Regolare il guadagno sulla parte più forte della sorgente.** Un livello adatto a un passaggio piano può saturare su un accento. Provare la sezione forte in `PAUSE+REC`, poi registrare. Ridurre successivamente il volume di riproduzione non elimina la distorsione registrata all'ingresso.

Il display distingue lo spazio per le registrazioni da quello per i file RAW. Una registrazione può entrare in memoria anche se non c'è spazio sufficiente per convertirla in RAW.

<a id="edit-a-recording-before-conversion-or-export"></a>

### Modificare una registrazione prima della conversione o dell'esportazione

1. Terminare con `STOP`, oppure selezionare una registrazione già completata in Sampler.
2. Premere **S1** per una registrazione mono. Per lo stereo, premere **S1** per il canale sinistro o **S2** per il destro.
3. In Sound Edit, suonare la registrazione dalla tastiera MIDI mentre se ne regolano i parametri di riproduzione.
4. Usare **From** e **To** per impostare il primo e l'ultimo campione, indicati dai limiti **A/B**. Usare Step per rifinire la regione.
5. Regolare intonazione, guadagno, pan, canale MIDI, curva di attacco, ADSR, modalità di riproduzione o Noclick secondo necessità.
6. Selezionare `RETURN` e premere Select per tornare a **SAMPLER**.

La sorgente resta la registrazione selezionata in Flash; l'audio non viene riscritto durante il ritaglio o l'editing e in questo editor non si può cambiare la sorgente. La tastiera riproduce la regione selezionata e resta utilizzabile durante le modifiche.

Per una registrazione stereo, ogni parametro modificato viene copiato automaticamente nell'altro canale: A/B, accordatura e Auto Tune, guadagno, pan, canale MIDI, curva di attacco, ADSR, modalità di riproduzione e Noclick. Le modifiche da uno qualsiasi dei canali agiscono su entrambi. Le posizioni pan originali sinistra/destra restano fino alla modifica del pan; cambiarlo o centrarlo applica lo stesso valore ai due canali.

L'editor Sampler offre solo `RETURN`, senza scelte separate per salvare o scartare. I limiti A/B vengono salvati automaticamente e conservati per riproduzione ed export successivi, anche dopo aver selezionato un'altra registrazione o riavviato LILLA. Anche uscendo dall'editor vengono mantenuti. Gli altri parametri restano attivi nella sessione di editing corrente, ma non vengono memorizzati come impostazioni permanenti della registrazione.

Conversione RAW ed export WAV usano la regione A/B salvata, estremi inclusi. Nello stereo, inizio e fine sono comuni ai due canali e ne mantengono allineata la durata. Queste operazioni copiano la regione audio selezionata; non applicano all'audio esportato intonazione, inviluppo, guadagno, pan o altri effetti di riproduzione dell'editor. Per esportare l'intera take, riportare prima A/B ai limiti dell'intera registrazione.

<a id="make-a-playable-raw-file"></a>

### Creare un file RAW suonabile

1. Selezionare la registrazione e, se necessario, usare S1/S2 per modificarne A/B, poi `RETURN`.
2. Scegliere `MAKE_RAW` e confermare la conversione.
3. Scegliere l'uscita disponibile: `MAKE_MONO`, `MAKE_LEFT`, `MAKE_RIGHT` o `MAKE_BOTH`.
4. Attendere il termine della conversione.
5. Aprire Sound Edit e scegliere la sorgente RAW generata.
6. Salvare la patch che la utilizza.

| Scelta di conversione | Risultato |
| --- | --- |
| `MAKE_MONO` | Crea una sorgente mono dalla registrazione. |
| `MAKE_LEFT` | Crea una sorgente dal canale sinistro della registrazione stereo. |
| `MAKE_RIGHT` | Crea una sorgente dal canale destro. |
| `MAKE_BOTH` | Crea sorgenti sinistra e destra separate. |

Le scelte disponibili dipendono dalla registrazione selezionata. Per lo stereo, sinistra e destra possono diventare sorgenti RAW separate. `CANCEL` esce dalle scelte di conversione. Se LILLA non riesce a creare un RAW, controllare memoria RAW libera e nomi di file disponibili.

La conversione copia la regione A/B salvata. Dopo la creazione riuscita di tutti i RAW richiesti, la registrazione Sampler originale viene eliminata e lo spazio liberato. Una conversione fallita conserva la registrazione. Effettuare prima un backup se si vuole mantenere la take originale.

<a id="export-a-wav-file"></a>

### Esportare un file WAV

Inserire una microSD, selezionare una registrazione e scegliere `EXPORT_WAV_TO_SD`. Il WAV contiene la regione A/B salvata, mantiene il formato mono o stereo e usa PCM a 16 bit e 44,1 kHz.

I file vengono scritti in `/LILLAWAV_EXPORT`, con nomi come `0M.wav` per il mono o `0S.wav` per lo stereo. Attendere il messaggio di successo prima di rimuovere la scheda.

Dopo un export WAV riuscito, LILLA elimina la registrazione Sampler originale e libera il suo slot e i pacchetti Flash. Se l'export fallisce, la registrazione resta. Per mantenerla in LILLA insieme a una copia esterna, usare invece il backup.

Sampler supporta fino a 30 registrazioni. Quando tutti gli slot sono occupati, `PAUSE+REC` viene nascosto e un avviso temporaneo chiede di eliminare o esportare una registrazione prima di crearne un'altra.

L'export WAV serve per modificare una take al computer, condividerla o conservarne una copia audio indipendente dalla configurazione di LILLA. Esporta le registrazioni Sampler; non è un comando generale di esportazione per tutte le sorgenti RAW in Flash.

`CANCEL_RECORDING` elimina la registrazione selezionata. Esportare prima ciò che si vuole conservare.

<a id="live-sampler"></a>

## Live Sampler

<img src="doc/assets/images/4.jpg" alt="Live Sampler prima della registrazione, con controlli di riproduzione e buffer" width="37%">

*La vista del buffer vuoto mostra capacità, guadagno d'ingresso, modalità di riproduzione, feedback e controlli del punto iniziale.*

Live Sampler registra in un buffer circolare PSRAM. Offre circa 40 secondi in mono o 20 in stereo. Proseguendo la registrazione, l'audio nuovo sostituisce quello più vecchio.

Il buffer live è temporaneo e si perde allo spegnimento. Per conservare un loop selezionato, usare la procedura di cattura in una patch descritta sotto.

<a id="record-and-explore"></a>

### Registrare ed esplorare

1. Impostare il selettore Modes su **Live Sampler**.
2. Scegliere `MONO/STEREO` prima di registrare. Cambiare questa impostazione cancella il buffer.
3. Selezionare `CAPTURE` per iniziare la registrazione dell'ingresso nel buffer live.
4. Selezionare `STOP` per fermare la registrazione e lavorare sull'audio registrato.
5. Scegliere una modalità di riproduzione e suonare dal controller MIDI.
6. Ruotare From per regolare l'inizio della regione e To per la sua lunghezza.
7. Ruotare Step per cambiare l'incremento di editing.

<a id="read-and-navigate-the-live-waveform"></a>

### Leggere e navigare la forma d'onda live

<img src="doc/assets/images/5.jpg" alt="Live Sampler con forma d'onda registrata e loop selezionato" width="37%">

*Qui la finestra visualizzata è di 1,3 secondi, mentre il loop selezionato è di 0,46 secondi. Cambiare lo zoom e modificare la lunghezza del loop sono operazioni distinte.*

| Campo | Significato |
| --- | --- |
| `BUFFER` | Capacità totale di registrazione live per il formato mono/stereo selezionato. |
| `LINE IN GAIN` | Guadagno applicato al segnale in ingresso da registrare. |
| `PLAY MODE` | Direzione e comportamento del loop. La cattura live in un suono richiede una modalità loop. |
| `FEEDBACK` | Quantità di materiale precedente reimmesso durante la registrazione live. |
| `WINDOW` | Durata dell'audio visibile nella vista della forma d'onda. |
| `START POINT` | Comportamento del punto iniziale e sua posizione o relazione con la registrazione. |
| `LOOP` | Durata della regione selezionata da ripetere. |
| `STEP` | Incremento usato per spostare i controlli della regione. |

Una Window più piccola aiuta a esaminare un transiente o un loop breve, ma non accorcia automaticamente l'audio selezionato. Usare **To** per la lunghezza del loop e **From** per spostarne l'inizio.

Iniziare con poco feedback e regolarlo ascoltando. Il feedback modifica il materiale registrato: confrontare il risultato prima di aumentarlo ancora.

Premere Step commuta il blocco del punto iniziale live. **FIXED** mantiene una posizione nel buffer circolare. Il comportamento sbloccato segue la posizione di registrazione e la pagina può mostrare **SYNC**, **BEHIND** o un'altra posizione relativa secondo lo scostamento.

Per la prima cattura, fermare la registrazione e lavorare su una regione fissa. Quando si ha familiarità con ricerca e ritaglio del materiale, provare un punto iniziale mobile durante la registrazione. Quando il buffer riparte dall'inizio, il materiale vecchio viene sostituito.

`ERASE` cancella il buffer registrato. Anche il cambio mono/stereo lo azzera: scegliere il formato prima di registrare materiale da conservare.

<a id="continuous-fwd-playback-while-recording"></a>

### Riproduzione FWD continua durante la registrazione

Durante la registrazione Live Sampler, le note tenute in **FWD** continuano a percorrere il buffer circolare invece di fermarsi dopo un giro. Vale per **SYNC**, posizioni iniziali relative e **FIXED**, in mono e stereo. Le note basse possono quindi restare attive oltre 40 secondi in mono (80 secondi a metà velocità). Note-off e inviluppo continuano a controllare la voce.

Durante il primo riempimento, raggiungere audio non ancora registrato provoca ancora la dissolvenza e l'arresto della voce. Quando la registrazione termina, FWD riprende il normale comportamento a riproduzione singola dalla posizione corrente. Le modalità reverse e loop restano invariate.

<a id="stereo-recording-compressor"></a>

### Compressore per la registrazione stereo

Nella pagina Live Sampler, ruotare **Select** per evidenziare il valore giallo `ON`/`OFF` accanto a `COMPRESSOR`, quindi premere **Select** per commutarlo. Il controllo è sulla riga `FEEDBACK`, allineato con `LOOP`. All'accensione parte **disattivato**; l'impostazione resta nella sessione corrente ma non viene salvata nella patch. Tutti i pulsanti **S1-S8** restano disponibili per catturare nei corrispondenti slot.

Il compressore agisce sulla somma di ingresso linea e feedback prima della scrittura nel buffer live. L'elaborazione audio funziona solo durante la registrazione Live Sampler, anche con le relative pagine Mixer o Delay aperte; altrimenti il blocco consuma gli ingressi senza allocare blocchi in uscita. L'impostazione ON/OFF resta memorizzata e la cronologia del lookahead viene azzerata alla ripresa della registrazione. Sinistra e destra condividono la stessa riduzione di guadagno, anche registrando un mix mono. La riduzione inizia intorno a -6 dBFS e, a piena attivazione, i picchi vengono limitati a circa -1 dBFS. Non corregge il clipping già avvenuto all'ingresso o altrove nel percorso di feedback e non elabora materiale già registrato.

Il lookahead di 128 campioni aggiunge circa 2,9 ms al percorso di registrazione, anche a compressore disattivato. La commutazione usa una transizione graduale di 10 ms tra segnali con lo stesso ritardo, senza salti nella sequenza temporale. Il recupero del guadagno richiede circa 100 ms per costante di tempo. Una compressione forte può comunque modificare suono e comportamento del feedback. Durante il bypass o la transizione da/verso il bypass, la protezione completa dai picchi non è garantita.

<a id="capture-a-live-loop-into-a-patch"></a>

### Catturare un loop live in una patch

1. Salvare le modifiche in sospeso della patch Performance prima di iniziare.
2. Registrare audio live, poi selezionare `STOP`.
3. Selezionare una modalità di riproduzione **loop** e rifinire la regione.
4. Premere il pulsante **S1-S8** dello slot desiderato.
5. Se si sostituisce uno slot di cattura occupato, rispondere a `REPLACE CAPTURE?` prima di continuare.
6. Quando richiesto, suonare una nota MIDI per impostare la nota di riferimento del suono catturato, oppure scegliere Cancel.
7. Se desiderato, catturare altre regioni in altri slot.
8. Passare a Performance, controllare la nuova patch e scegliere `SAVE`.
9. Attendere la conclusione delle scritture audio in sospeso prima di spegnere.

La prima cattura usa il primo ID di patch normale libero tra **0 e 199**. La patch **200** resta l'area temporanea di campionamento.

Ogni suono catturato inizialmente suona sulla nota usata alla richiesta della nota di riferimento: i limiti inferiore e superiore sono impostati su quella stessa nota. Ampliare l'intervallo in Performance per suonarlo melodicamente.

**Assegnazione degli slot stereo:** scegliere uno slot con il successivo libero, per esempio S1 con S2 libero, per catturare separatamente sinistra e destra. I due suoni sono posizionati a sinistra e a destra. Sostituire una coppia già catturata permette di riutilizzarla. Se non è disponibile un secondo slot, compresa una nuova cattura su S8, la regione viene catturata in un solo slot come mix mono dei due canali.

La regione selezionata deve entrare nella cache di cattura e devono essere disponibili risorse per patch, suoni e file. Il buffer live può essere più lungo di un singolo suono catturato: accorciare il loop se supera il limite di cattura.

L'audio catturato resta inizialmente in PSRAM. Salvare la patch scrive in Flash l'audio ancora in sospeso come file RAW. Attendere il termine; un salvataggio fallito deve essere riprovato prima di spegnere.

<a id="if-the-previous-patch-has-unsaved-edits"></a>

### Se la patch precedente contiene modifiche non salvate

Il messaggio:

> OPEN PERFORMANCE<br>
> AND SAVE THE PREVIOUS PATCH

si riferisce alla patch Performance usata prima di entrare in Live Sampler.

1. Fermare la registrazione live se è ancora attiva.
2. Tornare a Performance e completare l'eventuale conferma di uscita.
3. Salvare la patch precedente.
4. Tornare a Live Sampler.
5. Selezionare la regione desiderata e premere nuovamente il pulsante dello slot sonoro.

Non viene richiesto di salvare la patch 200. L'avviso protegge le modifiche della precedente patch normale prima di creare una nuova patch catturata.

<a id="capture-messages"></a>

### Messaggi di cattura

| Messaggio | Azione successiva |
| --- | --- |
| `NO RECORDED AUDIO` | Registrare audio in ingresso prima di riprodurlo o catturarlo. |
| `STOP REC AND SELECT LOOP MODE` | Fermare la registrazione e selezionare una modalità di riproduzione in loop. |
| `LOOP TOO LONG FOR CACHE` | Accorciare la regione selezionata. |
| `NO FREE PATCH` | Liberare uno slot di patch normale dopo aver conservato ciò che serve. |
| `NO FREE SOUND / CACHE / FILE` | Salvare il lavoro in sospeso e controllare le risorse disponibili per suoni, cache audio e file. |
| `CAPTURE CACHE UNAVAILABLE` | Non è stato possibile ottenere la memoria audio necessaria; conservare il lavoro in sospeso prima di riprovare. |
| `SAVE BUSY - TRY AGAIN` | Attendere che l'attività audio si stabilizzi e riprovare il salvataggio. |
| `RAW SAVE FAILED - RETRY` | Riprovare il salvataggio e mantenere acceso lo strumento finché resta audio catturato in sospeso. |

<a id="midi-loop"></a>

## MIDI Loop

<img src="doc/assets/images/8.jpg" alt="Pagina MIDI Loop con quattro tracce, livello, scostamento temporale e trasposizione" width="37%">

*Le quattro colonne sono tracce MIDI. Gli indicatori inferiori mostrano l'attività associata ai suoni della patch.*

MIDI Loop registra eventi MIDI su quattro tracce e li riproduce attraverso la patch corrente. Track 1 è la traccia master e stabilisce la durata del loop.

<a id="record-your-first-loop"></a>

### Registrare il primo loop

1. Inserire una microSD per salvare i loop.
2. Scegliere una patch e verificare che il controller suoni i suoni desiderati.
3. Impostare il selettore Modes su **MIDI Loop**.
4. Premere **Rec 1**, suonare la frase e premere nuovamente Rec 1 per terminare la registrazione.
5. Ascoltare la ripetizione della traccia master.
6. Premere Rec 2, Rec 3 o Rec 4 per registrare un'altra traccia; premere di nuovo lo stesso pulsante Rec per terminare.
7. Usare il menu per salvare il loop.

Registrare in una traccia occupata ne sostituisce gli eventi. **Registrare nuovamente Track 1 cancella anche le altre tracce**, perché crea un nuovo loop master. Salvare il loop esistente prima di sostituire la traccia master.

Una registrazione predisposta senza ricevere eventi viene annullata dopo circa 20 secondi.

<a id="add-parts-without-replacing-the-master"></a>

### Aggiungere parti senza sostituire la traccia master

Registrare prima su Track 1 la parte che definisce la lunghezza della frase. Quando si ripete correttamente, aggiungere una seconda parte su Track 2, poi proseguire con Track 3 e 4. Usare un canale MIDI diverso sul controller se la nuova parte deve indirizzare un altro suono in una patch multitimbrica.

Gli eventi registrati attivano i suoni attuali della patch. Cambiare sorgente, intervallo di tastiera o assegnazione MIDI può quindi modificare il risultato di un loop esistente. Conservare patch e libreria audio insieme al loop per poter riprodurre successivamente l'arrangiamento.

<a id="play-and-edit"></a>

### Riprodurre e modificare

- Ruotare **Loop** per scorrere i loop salvati.
- Premere Loop per fermare o riavviare il gruppo di tracce.
- Premere un encoder **Track** per fermare o avviare la singola traccia.
- Ruotare **Tempo** per modificare la temporizzazione; premerlo per azzerare la regolazione.
- Usare Select per scegliere la riga dei parametri delle tracce, poi ruotare ogni encoder Track per modificarne livello, intonazione o scostamento temporale.

<a id="understand-the-track-controls"></a>

### Comprendere i controlli delle tracce

| Riga | Cosa modifica | Valore iniziale |
| --- | --- | --- |
| `LEVEL` | Livello di riproduzione della traccia. | 1.0 per il livello non modificato. |
| `SHIFT` | Scostamento temporale della traccia all'interno del loop. | 0.00 secondi per nessuno scostamento. |
| `TRANSP` | Trasposizione delle note della traccia. | 0 tasti per le note originali. |

Usare un piccolo Shift per anticipare o ritardare una parte rispetto alle altre tracce. Ascoltare sia al confine del loop sia al centro della frase. Usare la trasposizione per provare un'intonazione diversa, poi riportarla a zero per confrontarla con l'originale.

Ruotare Tempo cambia la temporizzazione della sequenza MIDI. Non riscrive la forma d'onda di un campione e non applica automaticamente time-stretch a una frase audio registrata.

<a id="save-loops"></a>

### Salvare i loop

Usare `SAVE` per aggiornare un loop salvato e `SAVE_AS_NEW` per conservarne un'altra versione. `NEW` inizia un nuovo loop; `DELETE` elimina un loop salvato.

I file dei loop sono in `/LILLALOOP`. Conservare una copia di questa cartella quando si archivia il lavoro. Contengono dati MIDI: conservare anche la patch e l'audio sorgente necessari.

<a id="mixer-delay-and-filters"></a>

## Mixer, Delay e filtri

<a id="mixer"></a>

### Mixer

<img src="doc/assets/images/7.jpg" alt="Pagina Mixer con sorgenti sonore, ingresso linea e percorsi d'uscita separati" width="37%">

*La colonna evidenziata è la sorgente selezionata. LINEOUT e MONITOR sono percorsi separati.*

Aprire **Tools > Mixer** per regolare guadagno o mute, pan e assegnazione alle uscite linea e monitor. Ruotare Select per scegliere una colonna sorgente, poi premerlo per entrare nei campi. Ruotare Select per scegliere un campo e Value per modificarlo; premere Select per tornare alla selezione della sorgente.

Usare i percorsi separati linea e monitor per decidere cosa sente il pubblico e cosa si ascolta in monitoraggio. Se una sorgente è muta, controllare mute/guadagno e percorso d'uscita, oltre al volume della patch.

<a id="delay"></a>

### Delay

<img src="doc/assets/images/9.jpg" alt="Pagina Delay con routing dei suoni, feedback, tempo e modulazione stereo" width="37%">

*La riga ROUTING seleziona gli slot sonori inviati al delay. I valori di esempio non sono impostazioni predefinite consigliate.*

Aprire **Tools > Delay** per regolare feedback, tempo di ritardo, rapporto dei tempi sinistra/destra, sorgente, frequenza e profondità di modulazione e fase di modulazione sinistra/destra.

Iniziare con poco feedback, poi aumentarlo ascoltando. Se non si sente il segnale ritardato, controllare il routing del delay dello strumento. Salvare la patch per conservarne le impostazioni.

Per una prima prova, inviare uno strumento al delay, usare feedback moderato e un ritardo chiaramente udibile. Suonare note brevi separate da pause per sentire le ripetizioni. Regolare poi la differenza temporale sinistra/destra per la separazione stereo.

La modulazione varia il delay nel tempo. Aumentare gradualmente la profondità e regolare velocità e fase sinistra/destra ascoltando. Un feedback maggiore accumula le ripetizioni: ridurlo se il segnale ritardato sovrasta quello diretto.

<a id="filters-and-sound-character"></a>

### Filtri e carattere del suono

<img src="doc/assets/images/3.jpg" alt="Pagina VCF dello strumento con filtro passa-basso e modulazione LFO" width="37%">

*Il VCF individuale modella un solo strumento. La frequenza di taglio del LPF comune resta visibile sopra.*

Per raggiungere il VCF da Performance, premere il pulsante del suono attivo per aprire Sound Edit, poi premere nuovamente lo stesso pulsante. Usare Select per evidenziare un parametro del filtro e Value per modificarlo.

| Tipo di filtro | Effetto udibile |
| --- | --- |
| Lowpass | Attenua le frequenze sopra il taglio; utile per scurire una sorgente brillante. |
| Highpass | Attenua le frequenze basse; utile per alleggerire un suono o ridurne il peso sui bassi. |
| Bandpass | Evidenzia una regione tra frequenze basse e alte. |
| Notch | Elimina una banda di frequenze. |
| None | Disattiva il filtro dello strumento. |

La risonanza accentua la risposta del filtro intorno alla frequenza caratteristica. Iniziare con un valore moderato, poi ascoltare spostando il taglio. Sorgente e profondità di modulazione determinano se e come il parametro varia nel tempo; il campo frequenza/tempo segue il tipo di modulazione selezionato.



Il controllo comune **Cutoff** modifica il passa-basso. Premerlo per ripristinare il taglio massimo. **Resolution** e **Downsampling** aggiungono una colorazione digitale.

La pagina VCF offre tipo di filtro, taglio, risonanza e modulazione. Regolarli suonando il suono selezionato per ascoltarne l'interazione con campione e inviluppo.

Uno strumento bloccato è protetto da alcune modifiche di performance. Controllare `LOCK` se pitch bend, resolution o downsampling sembrano non avere effetto.

<a id="setup-and-midi-controls"></a>

## Setup e controlli MIDI

<img src="doc/assets/images/10.jpg" alt="Pagina Setup con convenzioni di accordatura, assegnazioni MIDI e operazioni di archiviazione" width="37%">

*Setup riunisce preferenze globali di esecuzione, gestione della libreria audio e backup.*

Aprire **Tools > Setup** per:

- `KEY STEP`: incrementi della mappatura d'intonazione di un semitono, mezzo semitono, un quarto o un ottavo di semitono.
- `FIRST OCTAVE`: convenzione di numerazione delle ottave visualizzate.
- `CONTROL CHANGE ASSIGNMENT`: assegnazioni MIDI CC ai guadagni dei suoni 1-8 e al taglio del passa-basso.
- Importazione audio, backup, ripristino e reset di fabbrica.

<a id="pitch-steps-and-note-names"></a>

### Intervalli di intonazione e nomi delle note

Con `KEY STEP` a un semitono, le note MIDI adiacenti usano la normale distanza cromatica. Passi minori distribuiscono un intervallo più piccolo a ogni tasto, permettendo di suonare per mezzi, quarti o ottavi di semitono. Tornare a un semitono per verificare una mappatura convenzionale.

`FIRST OCTAVE` cambia la numerazione delle ottave sul display. Aiuta ad allinearla alla convenzione del controller; non sostituisce l'impostazione della nota di riferimento dello strumento.

<a id="assign-a-controller-knob"></a>

### Assegnare un controllo del controller

<img src="doc/assets/images/11.jpg" alt="Pagina Control Change Assignment per gli otto guadagni dei suoni e il taglio LPF" width="37%">

*Ogni destinazione può avere un'assegnazione CC. Un trattino indica nessuna assegnazione.*

1. Aprire `CONTROL CHANGE ASSIGNMENT`.
2. Selezionare il guadagno di destinazione o il taglio LPF.
3. Impostare il numero CC desiderato.
4. Impostare la manopola o il cursore del controller MIDI per trasmettere quel CC.
5. Per il guadagno di un suono, usare il canale MIDI assegnato a quel suono.
6. Tornare e provare il controllo mentre si suona.

Usare un trattino per lasciare una destinazione non assegnata. Se il controllo esterno non ha effetto, verificare sia numero del controller sia canale di trasmissione.

<a id="check-incoming-midi"></a>

### Verificare i messaggi MIDI in ingresso

<img src="doc/assets/images/15.jpg" alt="Monitor MIDI con messaggio NoteOn, canale, nota e velocity" width="37%">

*L'esempio conferma la ricezione di un NoteOn sul canale 1. Il nome della nota segue la convenzione corrente per le ottave.*

Aprire **Tools > Test** per monitorare i tipi di messaggi MIDI in ingresso. Serve per verificare l'invio di note, pitch bend, aftertouch o control change dal controller.

Se il monitor riceve note ma Performance resta muta, la connessione funziona: controllare mappatura dei canali, intervalli e routing audio. Se non appare alcun messaggio, controllare uscita del controller, cavo e connessione selezionata prima di modificare la patch.

<a id="backup-and-restore"></a>

## Backup e ripristino

<a id="plan-a-complete-archive"></a>

### Preparare un archivio completo

Il backup della configurazione è una parte della conservazione di una sessione. Affiancargli la libreria audio corrispondente e i file dei loop MIDI, affinché le impostazioni salvate dispongano del materiale necessario.

| Elemento | Backup numerato di configurazione e registrazioni | Operazione aggiuntiva |
| --- | --- | --- |
| Patch, suoni e configurazione salvati | Inclusi. | Salvare le modifiche correnti prima del backup. |
| Associazioni dei nomi dei file | Incluse. | Non cambiare i nomi base dell'audio corrispondente. |
| Audio delle registrazioni Sampler | Incluso. | L'export WAV è utile anche per l'accesso da computer. |
| Limiti A/B delle registrazioni Sampler | Inclusi nei backup attuali. | I backup precedenti senza metadati di ritaglio ripristinano l'intera regione della registrazione. |
| Libreria RAW importata | Non inclusa. | Conservare separatamente la libreria originale d'importazione. |
| File RAW generati da registrazioni o catture live | Non inclusi come archivio completo della libreria RAW. | Conservare una copia audio indipendente e recuperabile; il solo salvataggio in Flash non è un backup esterno. |
| Cartella dei loop MIDI | Non inclusa. | Copiare `/LILLALOOP` dalla scheda. |
| Buffer live temporaneo | Non incluso. | Catturare e salvare il materiale utile prima di spegnere. |

Per una cattura Live Sampler, il comando di export WAV di Sampler non fornisce un'esportazione generale della libreria RAW. Non presumere che il solo backup numerato possa ripristinare ogni sorgente RAW catturata dopo che questa è stata cancellata.

<a id="create-a-backup"></a>

### Creare un backup

1. Salvare le modifiche della patch corrente.
2. Inserire una microSD con spazio libero sufficiente.
3. Aprire Tools > Setup.
4. Selezionare `NEW NUMBERED BACKUP IN /LILLABACKUP` e confermare.
5. Attendere il messaggio di backup riuscito.
6. Copiare la cartella del backup sul computer per conservarla.

I backup vengono creati in cartelle numerate come `/LILLABACKUP/000001`. Contengono configurazione e audio delle registrazioni Sampler.

Conservare separatamente la libreria sorgente importata. Il backup non copia tutta la libreria RAW importata, il buffer live temporaneo o la cartella dei loop MIDI. Copiare `/LILLALOOP` separatamente e conservare i file d'importazione. Salvare le catture live in una patch prima di archiviarne le impostazioni, tenendo conto del limite relativo all'audio RAW descritto sopra.

<a id="restore-a-backup"></a>

### Ripristinare un backup

<img src="doc/assets/images/13.jpg" alt="Conferma di ripristino che avvisa della sostituzione di patch, suoni e registrazioni" width="37%">

*Scegliere YES solo dopo aver preparato il backup desiderato nella cartella radice dei backup sulla scheda.*

1. Sul computer, scegliere il backup numerato da ripristinare.
2. Copiare **il contenuto** della cartella in `/LILLABACKUP` sulla scheda, mantenendo insieme configurazione e registrazioni.
3. Verificare l'esistenza di `/LILLABACKUP/LILLA_CONFIG.fram`. Lasciarlo soltanto dentro `000001`, per esempio, non basta.
4. Inserire la scheda e aprire Tools > Setup.
5. Selezionare `RESTORE CONFIG + AUDIO FROM /LILLABACKUP ROOT` e confermare.
6. Attendere il completamento del ripristino e dell'eventuale riavvio richiesto.
7. Controllare patch e registrazioni ripristinate e la disponibilità delle relative sorgenti importate.

La posizione di ripristino deve essere strutturata così:

```text
Radice microSD/
  LILLABACKUP/
    LILLA_CONFIG.fram
    [file audio delle registrazioni corrispondenti al backup scelto]
```

La riga tra parentesi quadre è una descrizione, non un nome di file da creare. Copiare i file reali delle registrazioni insieme alla configurazione; non mescolare file di backup numerati diversi.

Il ripristino sostituisce la configurazione e ripristina l'audio delle registrazioni. Fare un backup dello stato corrente prima di ripristinarne uno diverso. Mantenere intatto il backup originale: audio mancante o non valido può impedire il recupero di una registrazione.

<a id="factory-reset"></a>

### Ripristino delle impostazioni di fabbrica

<img src="doc/assets/images/14.jpg" alt="Conferma di reset di fabbrica nella pagina Setup" width="37%">

*Il reset di fabbrica è un'operazione distruttiva sulla configurazione, non un modo per uscire dall'editing.*

`FACTORY RESET` elimina patch, suoni e registrazioni. Effettuare un backup prima di confermare. Attendere la conclusione del reset e del riavvio.

<a id="updating-the-firmware-on-windows"></a>

## Aggiornare il firmware su Windows

Il firmware è il programma che fa funzionare LILLA. Per caricare un firmware compilato su Windows 10 o 11, usare **Teensy Loader (`teensy.exe`)**. È un'applicazione autonoma: scaricarla ed eseguirla senza installazione. **Teensyduino** è l'estensione di sviluppo per Arduino; né questa, né Arduino IDE o PlatformIO servono per caricare il file HEX fornito. La [pagina dei download PJRC](https://www.pjrc.com/teensy/td_download.html) distingue gli strumenti di sviluppo dal loader autonomo.

<a id="what-you-need"></a>

### Occorrente

- Lo strumento LILLA, che utilizza un Teensy 4.1.
- Un computer con Windows 10 o 11.
- Un cavo USB **dati** adatto al computer e al connettore USB-C di LILLA. Un cavo di sola ricarica non può trasferire il firmware.
- Il file firmware di LILLA, per esempio `Lilla_v7_0_2.hex`.
- Teensy Loader, scaricato da PJRC.

<a id="download-the-firmware"></a>

### Scaricare il firmware

1. Aprire il [ramo main del repository GitHub di LILLA](https://github.com/SandroGrassia/Lilla_Audio_Sampler/tree/main). Verificare che il selettore del ramo mostri **main**.
2. Nell'elenco principale dei file del progetto, aprire il `.hex` pubblicato per lo strumento, per esempio `Lilla_v7_0_2.hex`. È il firmware compilato; **Code > Download ZIP** scarica invece i sorgenti del progetto.
3. Nella pagina del file HEX, fare clic su **Download raw file**. Scaricare il firmware solo da **main**. Il ramo **develop** contiene lavoro in corso e può includere compilazioni difettose o non verificate.
4. Salvare il file in una cartella facilmente reperibile, come `Downloads/LILLA`. Verificare che il nome termini con `.hex`, non `.html` o `.txt`.

I numeri in `Lilla_v7_0_2.hex` identificano versione e revisione del firmware. Conservare la copia scaricata per mantenere esattamente quella compilazione. Se su **main** non è disponibile alcun HEX, attendere la pubblicazione da parte del manutentore; non sostituirlo con un file da **develop**.

<a id="download-teensy-loader"></a>

### Scaricare Teensy Loader

Aprire la [pagina ufficiale PJRC di Teensy Loader per Windows](https://www.pjrc.com/teensy/loader_win10.html) e fare clic su **Teensy Loader Program**. Salvare `teensy.exe` e aprirlo con un doppio clic. Dovrebbe apparire la piccola finestra di Teensy Loader. È possibile conservare l'eseguibile nella stessa cartella del file HEX. Le [istruzioni PJRC per il primo utilizzo](https://www.pjrc.com/teensy/first_use.html) spiegano che la modalità di programmazione usa i driver USB integrati in Windows: non occorre un driver di programmazione aggiuntivo.

<a id="prepare-lilla"></a>

### Preparare LILLA

Salvare la patch corrente e i loop MIDI e completare registrazioni o export in sospeso. Usare [Backup e ripristino](#backup-and-restore) per conservare il lavoro prima di cambiare firmware. Controllare eventuali istruzioni di compatibilità o migrazione fornite con la nuova versione.

Abbassare il livello dell'amplificatore o del mixer, poi collegare la USB-C di LILLA al computer con il cavo dati. Mantenere alimentazione e USB collegate per tutta la programmazione.

<a id="upload-and-restart"></a>

### Caricare e riavviare

1. In Teensy Loader, lasciare **Automatic Mode** disattivato per questa procedura manuale.
2. Scegliere **File > Open HEX File** e selezionare `Lilla_v7_0_2.hex`. Verificare il nome visualizzato nel loader.
3. Premere brevemente e rilasciare **Firmware_upload mode** di LILLA. È il pulsante di programmazione, non On/off. Il programma corrente dello strumento si ferma e il loader dovrebbe rilevare il Teensy.
4. Scegliere **Operations > Program**. Attendere **Download Complete** prima di scollegare qualsiasi cosa.
5. Scegliere **Operations > Reboot**. LILLA dovrebbe riavviarsi.
6. Controllare la versione del firmware nella schermata di benvenuto: dovrebbe essere 7.0.2. Caricare una patch nota e verificare la riproduzione a basso volume.

I comandi seguono le [istruzioni PJRC del loader per Windows](https://www.pjrc.com/teensy/loader_win10.html). L'aggiornamento programma la memoria interna del Teensy; l'importazione audio dalla SD è un'operazione distinta.

<a id="if-the-upload-does-not-start"></a>

### Se il caricamento non parte

| Sintomo | Controlli da effettuare |
| --- | --- |
| Il loader non rileva LILLA | Premere e rilasciare Firmware_upload mode dopo aver collegato USB. Provare un cavo dati funzionante e un'altra porta USB del computer. |
| Il file HEX non si apre | Scaricare nuovamente il file `.hex` grezzo. Verificare di non aver salvato la pagina web GitHub o un archivio dei sorgenti. |
| La programmazione termina ma LILLA non parte | Scegliere Operations > Reboot dopo Download Complete. Se necessario, ricollegare e ripetere con il firmware LILLA corretto. |
| La schermata di benvenuto mostra la vecchia versione | Controllare quale HEX è aperto nel loader e ripetere Program, poi Reboot. |

<a id="updating-the-firmware-on-mac"></a>

## Aggiornare il firmware su Mac

Su macOS, usare l'applicazione autonoma **Teensy Loader** e il file `.hex` di LILLA pubblicato. Arduino IDE, PlatformIO e l'estensione Teensyduino non servono per caricare un firmware compilato. PJRC elenca il loader autonomo nella propria [pagina dei download](https://www.pjrc.com/teensy/td_download.html).

<a id="what-you-need-on-mac"></a>

### Occorrente su Mac

- Lo strumento LILLA, che utilizza un Teensy 4.1.
- Un Mac compatibile con la versione attuale di Teensy Loader.
- Un cavo USB **dati** adatto al Mac e alla USB-C di LILLA. Un eventuale adattatore deve supportare il trasferimento dati USB.
- Il file HEX pubblicato di LILLA e l'applicazione Teensy Loader per macOS.

<a id="download-the-firmware-on-mac"></a>

### Scaricare il firmware su Mac

1. Nel browser, aprire il [ramo main del repository GitHub di LILLA](https://github.com/SandroGrassia/Lilla_Audio_Sampler/tree/main). Verificare che il selettore del ramo mostri **main**.
2. Aprire il `.hex` pubblicato nell'elenco principale dei file, per esempio `Lilla_v7_0_2.hex`.
3. Fare clic su **Download raw file** e salvarlo in una cartella comoda, come `Downloads/LILLA`.
4. Nel Finder, verificare che il file termini con `.hex`. Una pagina web GitHub o l'archivio sorgente **Code > Download ZIP** non possono essere caricati come firmware.

Usare soltanto firmware da **main**. I file di **develop** sono in lavorazione e possono essere difettosi o non verificati. Se main non contiene un HEX, attendere la pubblicazione. Conservare la copia scaricata per mantenere quella precisa compilazione.

<a id="download-and-open-teensy-loader-on-mac"></a>

### Scaricare e aprire Teensy Loader su Mac

1. Aprire la [pagina ufficiale PJRC del loader per Mac](https://www.pjrc.com/teensy/loader_mac.html) e scaricare **Teensy Loader Disk Image**.
2. Nel Finder, aprire il `.dmg` scaricato. Contiene l'applicazione Teensy Loader.
3. Copiare l'applicazione in **Applications** per riutilizzarla facilmente, poi aprirla. Confermare **Open** se macOS chiede conferma per l'applicazione scaricata.

Se macOS blocca un'applicazione non verificata, controllare prima che provenga dal download ufficiale PJRC. Dopo aver tentato di aprirla, usare **menu Apple > Impostazioni di Sistema > Privacy e sicurezza > Apri comunque**, quindi confermare **Apri**, se disponibile. Seguire le [istruzioni Apple per aprire app scaricate](https://support.apple.com/en-us/102445); non disattivare globalmente la sicurezza di macOS. Se il loader segnala un sistema non supportato, ottenere una versione compatibile da PJRC.

<a id="prepare-lilla-on-mac"></a>

### Preparare LILLA su Mac

Salvare patch e loop MIDI, terminare registrazioni o export in sospeso ed effettuare un backup tramite [Backup e ripristino](#backup-and-restore). Leggere le istruzioni di compatibilità o migrazione fornite con il firmware.

Abbassare il livello di ascolto. Collegare la USB-C di LILLA al Mac con il cavo dati e mantenere alimentazione e USB collegate durante la programmazione. Consentire la connessione dell'accessorio USB se il Mac lo richiede.

<a id="upload-and-restart-on-mac"></a>

### Caricare e riavviare su Mac

1. Lasciare **Automatic Mode** di Teensy Loader disattivato.
2. Scegliere **File > Open HEX File** e selezionare il file HEX di LILLA scaricato.
3. Premere brevemente e rilasciare **Firmware_upload mode** di LILLA, non On/off. Il loader dovrebbe rilevare il Teensy.
4. Scegliere **Operations > Program** e attendere **Download Complete**.
5. Scegliere **Operations > Reboot** per riavviare LILLA.
6. Controllare la versione nella schermata di benvenuto e provare una patch nota a basso volume. Per questa release deve apparire 7.0.2.

Questi comandi sono descritti nelle [istruzioni PJRC del loader per Mac](https://www.pjrc.com/teensy/loader_mac.html).

<a id="if-the-mac-cannot-upload"></a>

### Se il caricamento su Mac non riesce

| Sintomo | Controlli da effettuare |
| --- | --- |
| Teensy Loader non si apre | Controllare provenienza del download, richiesta di autorizzazione macOS e requisiti del loader. |
| LILLA non viene rilevata | Ricollegare USB, autorizzare l'accessorio se richiesto e premere brevemente Firmware_upload mode. Provare un altro cavo dati, porta o adattatore. |
| Il file HEX non si apre | Scaricare nuovamente l'HEX grezzo da main e verificarne l'estensione nel Finder. |
| La programmazione termina ma LILLA non si riavvia | Scegliere Operations > Reboot dopo Download Complete. |

<a id="troubleshooting"></a>

## Risoluzione dei problemi

<a id="diagnose-silence-in-a-useful-order"></a>

### Verificare sistematicamente l'assenza di suono

1. **MIDI:** Tools > Test mostra le note in ingresso?
2. **Mappatura:** un suono attivo è assegnato a quel canale e intervallo di note?
3. **Sorgente:** l'audio previsto è presente e la regione è valida?
4. **Inviluppo e livello:** guadagno e sustain sono sufficienti? L'attacco è insolitamente lungo?
5. **Routing:** la sorgente è attiva e assegnata all'uscita utilizzata?
6. **Uscita:** volume della patch, livello del mixer esterno e collegamento fisico sono corretti?

Cambiare una sola cosa alla volta e riprovare con la stessa nota. Questo facilita l'individuazione della causa reale.

| Problema | Controlli da effettuare |
| --- | --- |
| Nessun suono dal controller | Connessione MIDI, canale dello strumento, intervallo di tastiera, disponibilità della sorgente, guadagno, volume della patch e routing d'uscita del Mixer. |
| Uno slot non si apre per l'editing | Lo slot potrebbe essere inutilizzato. Modificare uno slot attivo o clonare uno strumento esistente in uno libero. |
| Il campione suona troppo acuto o troppo grave | Nota di riferimento, intonazione del suono, pitch bend del controller e KEY STEP in Setup. |
| L'audio satura o distorce | Ridurre il guadagno d'ingresso della registrazione o quello di riproduzione. Controllare livelli del Mixer, resolution e downsampling. |
| Clic al confine del loop | Rifinire From/To e regolare Noclick dove disponibile. |
| Suonano meno note del previsto | LILLA ha fino a 16 voci; i layer ne consumano più di una e un carico maggiore può ridurre la disponibilità. Controllare precedenza e densità dell'arrangiamento. |
| L'importazione SD non trova i file | Controllare la scheda e la cartella `/LILLA_AUDIO` nella sua radice, con quel nome esatto. |
| L'importazione rifiuta un WAV o AIFF | Usare PCM non compresso a 16 bit e 44,1 kHz, con uno o due canali. |
| L'importazione segnala duplicati | Assegnare nomi base distinti alle sorgenti, anche tra file di formati diversi. |
| Un campione importato termina prima del previsto | Controllare il limite d'importazione di circa 35,7 secondi. |
| La conversione RAW fallisce | Controllare la memoria libera per i RAW e i nomi di file disponibili. |
| L'export WAV fallisce | Controllare che la SD sia presente, scrivibile e con spazio libero. |
| L'audio live scompare | Il buffer live è temporaneo. Catturare il loop desiderato in una patch e salvarla prima di spegnere. |
| Live Sampler chiede di aprire Performance e salvare | La precedente patch normale ha modifiche non salvate. Tornare a Performance, salvarla e riprovare la cattura. |
| La cattura live viene rifiutata | Fermare la registrazione, selezionare una modalità loop, accorciare la regione se necessario e salvare le modifiche della patch esistente. |
| Le tracce scompaiono dopo aver registrato di nuovo Track 1 | Track 1 crea un nuovo loop e cancella le altre tracce. |
| Il ripristino non trova il backup | Inserire `LILLA_CONFIG.fram` e i file di registrazione corrispondenti direttamente in `/LILLABACKUP`. |
| Una patch ripristinata non riproduce la sorgente | Ripristinare o reimportare la libreria necessaria; il backup della configurazione non include tutto l'audio importato. |

<a id="practical-projects"></a>

## Progetti pratici

<a id="make-a-playable-instrument-from-a-recorded-note"></a>

### Creare uno strumento suonabile da una nota registrata

**Occorrente:** una sorgente collegata all'ingresso linea e un controller MIDI.

1. In Sampler, usare `PAUSE+REC` per regolare il livello d'ingresso.
2. Registrare una nota sostenuta pulita in mono, compresi attacco e decadimento.
3. Fermare, premere S1 per ascoltare dalla tastiera, regolare A/B e scegliere `RETURN`. Fare ora un backup se si vuole conservare l'intera registrazione originale.
4. Usare `MAKE_RAW` per creare una sorgente suonabile dalla regione scelta; la conversione riuscita rimuove la registrazione.
5. In Performance, clonare una patch da usare come punto di partenza.
6. Aprire un suono attivo e scegliere la nuova sorgente.
7. Ritagliare i silenzi indesiderati, scegliere una modalità di riproduzione e regolare l'inviluppo.
8. Impostare la nota di riferimento sulla nota registrata, poi definire l'intervallo di tastiera.
9. Suonare sopra e sotto la nota di riferimento per verificare il risultato.
10. Salvare la patch. Conservare il backup fatto prima della conversione se serve l'originale; una take convertita non è più disponibile per l'export WAV di Sampler.

**Provare anche:** clonare il suono in un altro slot, scegliere una regione diversa della stessa sorgente e assegnare zone di tastiera separate ai due slot.

<a id="turn-live-audio-into-a-small-playable-kit"></a>

### Creare un piccolo kit suonabile dall'audio live

**Occorrente:** diversi eventi brevi nel buffer Live Sampler e una patch Performance già salvata.

1. Registrare la sorgente in Live Sampler, poi fermare.
2. Selezionare una modalità loop e trovare il primo evento utile con From, To e Window.
3. Premere S1 e suonare la nota di attivazione desiderata quando richiesto.
4. Spostare la regione su un secondo evento.
5. Premere un altro slot libero e scegliere un'altra nota di attivazione.
6. Ripetere per gli altri eventi, prevedendo coppie di slot per le catture stereo.
7. Passare a Performance e verificare le assegnazioni delle note di attivazione.
8. Aprire ogni suono per scegliere modalità finale singola o loop e inviluppo.
9. Salvare la patch e attendere la scrittura dell'audio in Flash.

**Risultato:** una patch normale i cui slot contengono diverse regioni catturate. Il buffer live originale resta un'area di lavoro temporanea.

<a id="build-a-layered-texture-and-animate-it"></a>

### Creare e animare una texture sonora a più strati

1. Partire da una patch salvata e clonare un suono in uno slot libero.
2. Assegnare ai due slot lo stesso canale MIDI e intervalli di tastiera sovrapposti.
3. Usare regioni di ritaglio o regolazioni di intonazione diverse per i due layer.
4. Ridurre i guadagni individuali prima di ascoltare la combinazione.
5. Aprire il VCF di un layer e aggiungere una modulazione lenta con profondità moderata.
6. Inviare uno o entrambi i layer al Delay e aggiungere un po' di feedback.
7. Suonare note sostenute e ascoltare le code di rilascio.
8. Salvare la patch quando il bilanciamento è soddisfacente.

**Provare anche:** registrare una breve frase su Track 1 di MIDI Loop, aggiungere una parte contrastante su Track 2 e sperimentare un piccolo Shift temporale sulla seconda traccia.

<a id="quick-reference"></a>

## Riferimento rapido

| Operazione | Punto di partenza |
| --- | --- |
| Suonare una patch esistente | Performance > campo patch > Value. |
| Modificare un suono | Performance > S1-S8 per uno slot attivo. |
| Aggiungere uno strumento da un suono esistente | Sound Edit > CLONE, poi modificare il nuovo slot. |
| Conservare le modifiche dei suoni dopo lo spegnimento | Tornare a Performance > SAVE. |
| Importare audio dal computer | SD `/LILLA_AUDIO` > Tools > Setup > importazione. |
| Registrare l'ingresso linea | Sampler > PAUSE+REC > MONO_REC o STEREO_REC > STOP. |
| Modificare una registrazione suonandola dalla tastiera | Sampler > registrazione completata > S1 (mono/sinistra) o S2 (stereo destra) > Sound Edit > RETURN. |
| Conservare i punti di ritaglio per riproduzione ed export | Regolare A/B in Sound Edit di Sampler; salvataggio automatico e canali stereo collegati. |
| Trasformare una registrazione in una sorgente | Sampler > MAKE_RAW. |
| Esportare una registrazione | Sampler > EXPORT_WAV_TO_SD > SD `/LILLAWAV_EXPORT`. |
| Registrare audio live temporaneo | Live Sampler > CAPTURE > STOP. |
| Conservare un loop live | Fermare la registrazione live > modalità loop > S1-S8 > impostare la nota di riferimento > salvare la patch. |
| Registrare un loop MIDI | MIDI Loop > Rec 1 > suonare > nuovamente Rec 1. |
| Fare un backup di configurazione e registrazioni | Tools > Setup > nuovo backup numerato. |
| Ripristinare un backup numerato | Copiarne il contenuto in `/LILLABACKUP` sulla SD > Tools > Setup > ripristino. |

<a id="glossary"></a>

## Glossario

| Termine | In questo manuale |
| --- | --- |
| ADSR | Attacco, decadimento, sustain e rilascio: le fasi che modellano il livello di un suono nel tempo. |
| Nome base | Nome di un file senza estensione, come `BassDry` in `BassDry.wav`. |
| Capture | Nel menu Live Sampler, registrare nel buffer live; con S1-S8, copiare una regione selezionata in un suono della patch. |
| Buffer circolare | Memoria di registrazione che riparte dall'inizio e sovrascrive il materiale più vecchio. |
| Flash | Memoria audio persistente interna a LILLA. |
| FRAM | Memoria persistente per configurazione e metadati di patch e suoni. |
| LFO | Oscillatore a bassa frequenza usato per variare un parametro nel tempo. |
| LPF | Filtro passa-basso, che attenua le frequenze più alte. |
| MIDI CC | Messaggio MIDI Control Change usato per controllare un parametro assegnato. |
| Mono | Un canale audio. |
| Noclick | Smussamento dei limiti per ridurre le discontinuità nelle regioni di loop adatte. |
| PCM | Dati digitali di campioni audio non compressi. |
| Polifonia | Numero di voci di riproduzione simultanee; anche layer e code di rilascio consumano voci. |
| PSRAM | Memoria di lavoro audio; il contenuto si perde allo spegnimento. |
| RAW | Dati audio senza intestazione del contenitore WAV o AIFF. |
| Nota di riferimento | Nota MIDI di riferimento per la mappatura dell'intonazione di un suono. |
| Stereo | Due canali audio, sinistro e destro. |
| VCF | Filtro dello strumento, con controlli di taglio, risonanza e modulazione. |

---

*Lilla Manuale Utente - Edizione italiana - 8 ottobre 2026*

*Immagini del manuale: [doc/assets/images](doc/assets/images/). Mantenere questo percorso relativo quando si condivide il manuale illustrato.*
