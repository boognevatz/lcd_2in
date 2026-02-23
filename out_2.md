⏺ Scenario 4, tick 10 — change column A from PD to -:

  |   10 | F2L->C 1/2   | "B+a | -      | TXP    | REC    | F1L | F2U | F1U |

  Scenario 4, tick 11 — change column A from TXP to TX:

  |   11 | F2L->C 2/2   | "b+A | TX     | -      | REC    | F1L | F2U | F2L |

  Root cause: F1L in bucket A was transmitted during TX REC at tick 7 (camera and TX both finished A simultaneously). TX REC counts as "sent" — confirmed by Scenario 3 tick 2, where identical logic correctly shows - instead of PD. Since F1L is stale/already-sent, A doesn't qualify for PD protection at pair selection (tick 10), and consequently gets TX (not TXP)
  when TX sends it at tick 11.
