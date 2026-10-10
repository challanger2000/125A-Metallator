# Provisorisches VSTGUI – Metallator V1

Native Steinberg VSTGUI UIDescription mit VST3 Parameter-Binding; UI-Entwurf, kein finales 125A GUI und keine Freigabe.

- IMPACT/PERC als Zwei-Segment-Auswahl, 5 Regler SIZE, FORCE, CHAOS, DECAY, LEVEL; GENERATE, VARIATE und genau ein BYPASS. Keine FRlCTION/DRONE-Engines, keine tote KEYTRACK-Bedienung.
- 940 x 478 px bei 100%; VST3Editor-Kontextmenü gestattet Zoom 100/150% (1410 x 717 px bei 150%).
- GENERATE/VARIATE sind **Momenttasten** mit steigender Flanke; Loslassen löst keine zweite Generation aus. Bypass ist ein **rastender** Schalter.
- Kein Timer, kein GUI->DSP-Pointer, keine Dateizugriffe aus Audio-Callback, unveränderte Parameter-IDs. DSP-Archetypen und State-Schema bleiben unverändert.
- Renderte Knopf-Filmstrips und autoritatives 125A SVG-Logo werden erst für das finale Branding eingesetzt. Diese Übergangsversion enthält bewusst keine nachgezeichneten Logos oder falschen Preset-/Export-Funktionen.

QA-Gates offen: Windows VSTGUI/SDK Compile, Template-Load, Editor Lifecycle, Parameterbindung, echtes Studio One, Screenshots bei 100/150%, Host-Zustand/Speichern, Realtime. Kein vollwertiger Produktstatus.
