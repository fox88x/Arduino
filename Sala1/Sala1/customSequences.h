// Sequenza 1: ManualLight_ON
SequenceStep sequence1[] = {
  SequenceStep(0x000000, 1000),   
  SequenceStep(0xFFFF00, 3000),  
  SequenceStep(0x000000, 600), 
  SequenceStep(0x00FF00, 500),  
  SequenceStep(0x000000, 500),  
  SequenceStep(0x00FF00, 500),  
  SequenceStep(0x000000, 500),   
  SequenceStep(0x00FF00, 500),  
  SequenceStep(0x000000, 1000)  
};

const int numSteps1 = sizeof(sequence1) / sizeof(sequence1[0]);


// Sequenza 2: ManualLight_OFF
SequenceStep sequence2[] = {
  SequenceStep(0x000000, 1000),  // Spento per 1s
  SequenceStep(0xFFFF00, 3000),  // Giallo per 3 sec
  SequenceStep(0x000000,  600),  // Spento per 600ms
  SequenceStep(0xFF0000,  500),  // Rosso per 500ms
  SequenceStep(0x000000,  500),  // Spento per 500ms
  SequenceStep(0xFF0000,  500),  // Rosso per 500ms
  SequenceStep(0x000000,  500),  // Spento per 500ms
  SequenceStep(0xFF0000,  500),  // Rosso per 500ms
  SequenceStep(0x000000, 1000)   // Spento per 1s
};

const int numSteps2 = sizeof(sequence2) / sizeof(sequence2[0]);


// Sequenza 3: AutoW_ON
SequenceStep sequence3[] = {
  SequenceStep(0x000000, 1000),  // Spento per 1s
  SequenceStep(0x0000FF, 3000),  // Blu per 3 sec
  SequenceStep(0x000000, 600),   // Spento per 600ms
  SequenceStep(0x00FF00, 500),   // Verde per 500ms
  SequenceStep(0x000000, 500),   // Spento per 500ms
  SequenceStep(0x00FF00, 500),   // Verde per 500ms
  SequenceStep(0x000000, 500),   // Spento per 500ms
  SequenceStep(0x00FF00, 500),   // Verde per 500ms
  SequenceStep(0x000000, 1000)   // Spento per 1s
};


const int numSteps3 = sizeof(sequence3) / sizeof(sequence3[0]);

// Sequenza 4: AutoW_OFF
SequenceStep sequence4[] = {
  SequenceStep(0x000000, 1000), // Spento per 1s
  SequenceStep(0x0000FF, 3000), // Blu per 3 sec
  SequenceStep(0x000000, 600),  // Spento per 600ms
  SequenceStep(0xFF0000, 500),  // Rosso per 500ms
  SequenceStep(0x000000, 500),  // Spento per 500ms
  SequenceStep(0xFF0000, 500),  // Rosso per 500ms
  SequenceStep(0x000000, 500),  // Spento per 500ms
  SequenceStep(0xFF0000, 500),  // Rosso per 500ms
  SequenceStep(0x000000, 1000)  // Spento per 1s
};


const int numSteps4 = sizeof(sequence4) / sizeof(sequence4[0]);

// Sequenza 5: Clock Syn OK
SequenceStep sequence5[] = {
  SequenceStep(0x000000, 1000),   // Spento per 1s
  SequenceStep(0x33FFFF, 3000),   // Ciano per 3 sec
  SequenceStep(0x000000, 600),    // Spento per 600ms
  SequenceStep(0x00FF00, 500),    // Verde per 500ms
  SequenceStep(0x000000, 500),    // Spento per 500ms
  SequenceStep(0x00FF00, 500),    // Verde per 500ms
  SequenceStep(0x000000, 500),    // Spento per 500ms
  SequenceStep(0x00FF00, 500),    // Verde per 500ms
  SequenceStep(0x000000, 1000)    // Spento per 1s
};

const int numSteps5 = sizeof(sequence5) / sizeof(sequence5[0]);


// Sequenza 6: Clock Syn OFF
SequenceStep sequence6[] = {
  SequenceStep(0x000000, 1000),   // Spento per 1s
  SequenceStep(0x33FFFF, 3000),   // Ciano per 3 sec
  SequenceStep(0x000000, 600),    // Spento per 600ms
  SequenceStep(0xFF0000, 500),    // Rosso per 500ms
  SequenceStep(0x000000, 500),    // Spento per 500ms
  SequenceStep(0xFF0000, 500),    // Rosso per 500ms
  SequenceStep(0x000000, 500),    // Spento per 500ms
  SequenceStep(0xFF0000, 500),    // Rosso per 500ms
  SequenceStep(0x000000, 1000)    // Spento per 1s
};

const int numSteps6 = sizeof(sequence6) / sizeof(sequence6[0]);


// Sequenza 7: Cloud OFF
SequenceStep sequence7[] = {
  SequenceStep(0,   0,   0,   400),
  SequenceStep(255, 0,   0,   400),  
  SequenceStep(0  , 0,   0,   400),  
  SequenceStep(255, 0,   0,   400),  
  SequenceStep(0  , 0,   0,   400),  
  SequenceStep(255, 0,   0,   400), 
  SequenceStep(0,   0,   0,   400),  
  SequenceStep(255, 0,   0,   400),  
  SequenceStep(0,   0,   0,   400)
};

const int numSteps7 = sizeof(sequence7) / sizeof(sequence7[0]);

// Sequenza 8: Cloud SYN
SequenceStep sequence8[] = {
  SequenceStep(0,0,   0, 400),
  SequenceStep(0,0, 255, 400),  
  SequenceStep(0,0,   0, 400),  
  SequenceStep(0,0, 255, 400),  
  SequenceStep(0,0,   0, 400),  
  SequenceStep(0,0, 255, 400), 
  SequenceStep(0,0,   0, 400),  
  SequenceStep(0,0, 255, 400),  
  SequenceStep(0,0,   0, 400)  
};

const int numSteps8 = sizeof(sequence8) / sizeof(sequence8[0]);


// Sequenza 9: Cloud CONN
SequenceStep sequence9[] = {
  SequenceStep(0,0,   0, 400),
  SequenceStep(0, 255,   0,  400),  
  SequenceStep(0,   0,   0,  400),  
  SequenceStep(0, 255,   0,  400),  
  SequenceStep(0,   0,   0,  400),  
  SequenceStep(0, 255,   0,  400), 
  SequenceStep(0,   0,   0,  400),  
  SequenceStep(0, 255,   0,  400),  
  SequenceStep(0,   0,   0,  400)  
};

const int numSteps9 = sizeof(sequence9) / sizeof(sequence9[0]);


// Sequenza 10: Periodic Clock Syn
SequenceStep sequence10[] = {
  SequenceStep(0x000000, 1000),   // Spento per 1s
  SequenceStep(0x33FFFF, 500),    // Verde per 500ms
  SequenceStep(0x000000, 500),    // Spento per 500ms
  SequenceStep(0x33FFFF, 500),    // Verde per 500ms
  SequenceStep(0x000000, 500),    // Spento per 500ms
  SequenceStep(0x33FFFF, 500),    // Verde per 500ms
  SequenceStep(0x000000, 1000)    // Spento per 1s
};

const int numSteps10 = sizeof(sequence10) / sizeof(sequence10[0]);


// Sequenza 11: All command 
SequenceStep sequence11[] = {
  SequenceStep(0x000000, 1000),   // Spento per 1s
  SequenceStep(0x0000FF, 500),    // Blu per 500ms
  SequenceStep(0x000000, 500),    // Spento per 500ms
  SequenceStep(0x0000FF, 500),    // Blu per 500ms
  SequenceStep(0x000000, 1000),   // Spento per 500ms
};

const int numSteps11 = sizeof(sequence11) / sizeof(sequence11[0]);


// Sequenza 22: Arcobaleno
SequenceStep sequence22[] = {
  SequenceStep(0xFF0000, 500),  // Rosso per 500ms
  SequenceStep(0xFF7F00, 500),  // Arancione per 500ms
  SequenceStep(0xFFFF00, 500),  // Giallo per 500ms
  SequenceStep(0x00FF00, 500),  // Verde per 500ms
  SequenceStep(0x0000FF, 500),  // Blu per 500ms
  SequenceStep(0x4B0082, 500),  // Indaco per 500ms
  SequenceStep(0x9400D3, 500)   // Viola per 500ms
};

const int numSteps22 = sizeof(sequence22) / sizeof(sequence22[0]);

// Sequenza 3: Fade In
SequenceStep sequence32[] = {
  SequenceStep(0x000000, 100),  // Spento per 100ms
  SequenceStep(0x202020, 100),  // Luce bassa per 100ms
  SequenceStep(0x404040, 100),
  SequenceStep(0x606060, 100),
  SequenceStep(0x808080, 100),
  SequenceStep(0xA0A0A0, 100),
  SequenceStep(0xC0C0C0, 100),
  SequenceStep(0xFFFFFF, 100)  // Massima luminosità bianca per 100ms
};

const int numSteps32 = sizeof(sequence32) / sizeof(sequence32[0]);

// Sequenza 4: Flashing
SequenceStep sequence42[] = {
  SequenceStep(0xFFFFFF, 500),  // Acceso per 500ms
  SequenceStep(0x000000, 500),  // Spento per 500ms
  SequenceStep(0xFFFFFF, 500),  // Acceso per 500ms
  SequenceStep(0x000000, 500)   // Spento per 500ms
};

const int numSteps42 = sizeof(sequence42) / sizeof(sequence42[0]);

// Sequenza 5: Ritmica
SequenceStep sequence52[] = {
  SequenceStep(0xFF0000, 300),  // Rosso per 300ms
  SequenceStep(0x00FF00, 300),  // Verde per 300ms
  SequenceStep(0xFF0000, 300),  // Rosso per 300ms
  SequenceStep(0x00FF00, 300)   // Verde per 300ms
};

const int numSteps52 = sizeof(sequence52) / sizeof(sequence52[0]);
