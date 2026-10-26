# YXC API coverage checklist

Tracks implementation against Yamaha **YXC API Specification (Basic)** and
**(Advanced)** Rev. 2.00. Official PDFs are not redistributed here; see
[YXC-specs.md](YXC-specs.md).

**Legend**

| Column | Meaning |
|--------|---------|
| **Lib** | HTTP wrapper in `libmusiccast` (`src/musiccast.c` / `request.c`) |
| **CLI** | User-facing `mcctl` command (or subcommand) |
| **Extra** | Not a YXC HTTP API (e.g. SSDP discovery) |

Use `[x]` / `[ ]` when updating this file.

**Summary (HTTP APIs only)**

| Spec | Implemented in Lib | Exposed in mcctl |
|------|-------------------|----------------|
| Basic §4 System | 6 / 42 | 6 / 42 |
| Basic §5 Zone | 4 / 29 | 4 / 29 (+ `zones` parses `getFeatures`) |
| Basic §6 Tuner | 0 / 15 | 0 / 15 |
| Basic §7 Net/USB | 4 / 24 | 4 / 24 |
| Basic §8 CD | 0 / 6 | 0 / 6 |
| Basic §9 Clock | 0 / 5 | 0 / 5 |
| Basic §11 Events | 0 | 0 |
| Advanced §4–§5 | 0 / 9 | 0 / 9 |

---

## Extra (not in YXC HTTP spec)

| Feature | Lib | CLI | Notes |
|---------|-----|-----|-------|
| SSDP discovery | [x] | [x] | `musiccast_discover()` / `mcctl detect` |
| Zone list from features | [x] | [x] | `musiccast_zone_list()` / `mcctl zones` |
| `mcctl help` | — | [x] | Usage only |

---

## Basic §4 — System (`/v1/system/…`)

### Queries

| API | Lib | CLI | mcctl command |
|-----|-----|-----|----------------|
| getDeviceInfo | [x] | [x] | `device-info` |
| getFeatures | [x] | [x] | `features` |
| getNetworkStatus | [x] | [x] | `network-status` |
| getFuncStatus | [x] | [x] | `func-status` |
| getLocationInfo | [x] | [x] | `location-info` |
| getMacAddressFilter | [ ] | [ ] | |
| getNetworkStandby | [ ] | [ ] | |
| getBluetoothInfo | [ ] | [ ] | |
| getBluetoothDeviceList | [ ] | [ ] | |
| getNameText | [ ] | [ ] | |
| getStereoPairInfo | [ ] | [ ] | |
| getRemoteInfo | [ ] | [ ] | |
| getAdvancedFeatures | [ ] | [ ] | Useful before Link / groups (see Advanced §9) |

### Settings & control

| API | Lib | CLI | Notes |
|-----|-----|-----|-------|
| setWiredLan | [ ] | [ ] | POST body |
| setWirelessLan | [ ] | [ ] | POST body |
| setWirelessDirect | [ ] | [ ] | POST body |
| setIpSettings | [ ] | [ ] | POST body |
| setNetworkName | [ ] | [ ] | POST body |
| setAirPlayPin | [ ] | [ ] | |
| setMacAddressFilter | [ ] | [ ] | POST body |
| setNetworkStandby | [ ] | [ ] | |
| setBluetoothStandby | [ ] | [ ] | |
| setBluetoothTxSetting | [ ] | [ ] | |
| updateBluetoothDeviceList | [ ] | [ ] | |
| connectBluetoothDevice | [ ] | [ ] | |
| disconnectBluetoothDevice | [ ] | [ ] | |
| setAutoPowerStandby | [ ] | [ ] | |
| setIrSensor | [ ] | [ ] | |
| setSpeakerA | [ ] | [ ] | |
| setSpeakerB | [ ] | [ ] | |
| setDimmer | [ ] | [ ] | |
| setZoneBVolumeSync | [ ] | [ ] | |
| setHdmiOut1 | [ ] | [ ] | |
| setHdmiOut2 | [ ] | [ ] | |
| setHdmiOut3 | [ ] | [ ] | |
| setNameText | [ ] | [ ] | POST body |
| sendIrCode | [ ] | [ ] | |
| requestNetworkReboot | [ ] | [ ] | |
| requestSystemReboot | [ ] | [ ] | |
| setAutoPlay | [ ] | [ ] | |
| setSpeakerPattern | [ ] | [ ] | |
| setPartyMode | [ ] | [ ] | |

### Related (zone path in spec, implemented via system in lib)

| API | Lib | CLI | Notes |
|-----|-----|-----|-------|
| getSoundProgramList | [x] | [x] | Lib uses `/v1/system/getSoundProgramList`; spec also documents `/v1/<zone>/getSoundProgramList` |

---

## Basic §5 — Zone (`/v1/<zone>/…`)

| API | Lib | CLI | mcctl command |
|-----|-----|-----|----------------|
| getStatus | [x] | [x] | `status` |
| getSoundProgramList | [ ] | [ ] | See system path above |
| setPower | [x] | [x] | `power` |
| setSleep | [ ] | [ ] | |
| setVolume | [x] | [x] | `volume` |
| setMute | [x] | [x] | `mute` |
| setInput | [ ] | [ ] | |
| setSoundProgram | [ ] | [ ] | |
| set3dSurround | [ ] | [ ] | |
| setDirect | [ ] | [ ] | |
| setPureDirect | [ ] | [ ] | |
| setEnhancer | [ ] | [ ] | |
| setToneControl | [ ] | [ ] | |
| setEqualizer | [ ] | [ ] | |
| setBalance | [ ] | [ ] | |
| setDialogueLevel | [ ] | [ ] | |
| setDialogueLift | [ ] | [ ] | |
| setClearVoice | [ ] | [ ] | |
| setSubwooferVolume | [ ] | [ ] | |
| setBassExtension | [ ] | [ ] | |
| getSignalInfo | [ ] | [ ] | |
| prepareInputChange | [ ] | [ ] | |
| recallScene | [ ] | [ ] | |
| setContentsDisplay | [ ] | [ ] | |
| controlCursor | [ ] | [ ] | |
| controlMenu | [ ] | [ ] | |
| setActualVolume | [ ] | [ ] | |
| setAudioSelect | [ ] | [ ] | |
| setSurroundDecoderType | [ ] | [ ] | |

---

## Basic §6 — Tuner (`/v1/tuner/…`)

| API | Lib | CLI | Notes |
|-----|-----|-----|-------|
| getPresetInfo | [ ] | [ ] | |
| getPlayInfo | [ ] | [ ] | |
| setBand | [ ] | [ ] | |
| setFreq | [ ] | [ ] | |
| recallPreset | [ ] | [ ] | Not the same as `mcctl preset play` (netusb) |
| switchPreset | [ ] | [ ] | |
| storePreset | [ ] | [ ] | |
| clearPreset | [ ] | [ ] | |
| startAutoPreset | [ ] | [ ] | |
| cancelAutoPreset | [ ] | [ ] | |
| movePreset | [ ] | [ ] | |
| startDabInitialScan | [ ] | [ ] | |
| cancelDabInitialScan | [ ] | [ ] | |
| setDabTuneAid | [ ] | [ ] | |
| setDabService | [ ] | [ ] | |

---

## Basic §7 — Network/USB (`/v1/netusb/…`)

| API | Lib | CLI | mcctl command |
|-----|-----|-----|----------------|
| getPresetInfo | [x] | [x] | `preset list` |
| recallPreset | [x] | [x] | `preset play` / `preset recall` |
| storePreset | [x] | [x] | `preset store` |
| clearPreset | [x] | [x] | `preset clear` |
| getPlayInfo | [ ] | [ ] | |
| setPlayback | [ ] | [ ] | play / pause / stop / skip |
| setPlayPosition | [ ] | [ ] | |
| setRepeat | [ ] | [ ] | |
| setShuffle | [ ] | [ ] | |
| toggleRepeat | [ ] | [ ] | |
| toggleShuffle | [ ] | [ ] | |
| getListInfo | [ ] | [ ] | browse libraries |
| setListControl | [ ] | [ ] | |
| setSearchString | [ ] | [ ] | |
| setListSortOption | [ ] | [ ] | |
| manageList | [ ] | [ ] | POST body |
| managePlay | [ ] | [ ] | POST body |
| getRecentInfo | [ ] | [ ] | |
| recallRecentItem | [ ] | [ ] | |
| clearRecentInfo | [ ] | [ ] | |
| getPlayDescription | [ ] | [ ] | |
| getServiceInfo | [ ] | [ ] | |
| getAccountStatus | [ ] | [ ] | |
| getSettings | [ ] | [ ] | |
| setQuality | [ ] | [ ] | |
| movePreset | [ ] | [ ] | |

---

## Basic §8 — CD (`/v1/cd/…`)

| API | Lib | CLI |
|-----|-----|-----|
| getPlayInfo | [ ] | [ ] |
| setPlayback | [ ] | [ ] |
| setRepeat | [ ] | [ ] |
| setShuffle | [ ] | [ ] |
| toggleRepeat | [ ] | [ ] |
| toggleShuffle | [ ] | [ ] |
| toggleTray | [ ] | [ ] |

---

## Basic §9 — Clock (`/v1/clock/…`)

| API | Lib | CLI |
|-----|-----|-----|
| getSettings | [ ] | [ ] |
| setDateAndTime | [ ] | [ ] |
| setClockFormat | [ ] | [ ] |
| setAutoSync | [ ] | [ ] |
| setAlarmSettings | [ ] | [ ] |

---

## Basic §10 — Response codes

| Item | Lib | CLI |
|------|-----|-----|
| Parse / surface `response_code` in errors | [ ] | [ ] | Requests succeed/fail loosely today; JSON errors not unified |

---

## Basic §11 — Events

| Item | Lib | CLI | Notes |
|------|-----|-----|-------|
| HTTP event subscription | [ ] | [ ] | |
| UDP event receive / parse | [ ] | [ ] | Spec: poll on loss (e.g. `getStatus`, `getDistributionInfo`) |
| Polling helpers | [ ] | [ ] | Could be a future `mcctl watch` |

---

## Advanced §4 — Zone Link settings (`/v1/<zone>/…`)

| API | Lib | CLI | Notes |
|-----|-----|-----|-------|
| setLinkControl | [ ] | [ ] | Values from `getFeatures` |
| setLinkAudioDelay | [ ] | [ ] | |
| setLinkAudioQuality | [ ] | [ ] | Optional `/secure/` path in spec |

---

## Advanced §5 — Distribution (`/v1/dist/…`)

| API | Lib | CLI | Notes |
|-----|-----|-----|-------|
| getDistributionInfo | [ ] | [ ] | Poll during Link / events |
| setServerInfo | [ ] | [ ] | POST JSON (master + clients) |
| setClientInfo | [ ] | [ ] | POST JSON (per device) |
| startDistribution | [ ] | [ ] | `?num=` system distribution number |
| stopDistribution | [ ] | [ ] | |
| setGroupName | [ ] | [ ] | POST JSON |

### Advanced §8–§9 (procedural, not single endpoints)

| Workflow | Lib | CLI | Notes |
|----------|-----|-----|-------|
| MusicCast network initial setup | [ ] | [ ] | Join location / first-time network |
| Create / extend Link group | [ ] | [ ] | `setClientInfo` → `setServerInfo` → `startDistribution` |
| Remove client / dissolve group | [ ] | [ ] | |
| Multi-group | [ ] | [ ] | |
| Stereo pair via distribution | [ ] | [ ] | See Advanced §9.3 |
| Group volume behaviour | [ ] | [ ] | Advanced §9.1.9; may use Basic volume APIs |
| Distribution UDP events | [ ] | [ ] | Advanced §9.2 |

---

## Suggested implementation order

1. **Net/USB transport** — `getPlayInfo`, `setPlayback` (+ CLI `play` / `pause` / `stop`).
2. **Zone source** — `setInput`, `setSoundProgram`.
3. **Distribution** — `getDistributionInfo` first, then POST wrappers for server/client/start/stop.
4. **Tuner** — parallel to netusb presets under `mcctl tuner …`.
5. **System reads** — `getAdvancedFeatures`, `getNameText`, Bluetooth info (before touching Link).
6. **Events** — optional UDP listener or documented polling mode.

---

## Maintenance

When adding an API:

1. Implement in `src/musiccast.c` (and declare in `include/musiccast.h`).
2. Add `mcctl` command in `tools/mcctl.c` and `tools/mcctl.1.in`.
3. Flip `[ ]` → `[x]` in the tables above and adjust summary counts.
