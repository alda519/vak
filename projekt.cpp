#include <simlib.h>
#include <ctime>

// pracovni doba [hod]
#define WORKING_HOURS 7.5
// delka simulace [dny]
#define DAYS 50

// pocet posty - prumerne [denne]
#define POST_AVR 30
// rozptyl poctu posty
#define POSR_VAR 5
// prumerne prichozu lidi osobne [denne]
#define PEOPLE 10
// prumerne prichozich elektronickych zprav [denne]
#define EMAIL 20


Facility sekretarka("Sekretarka");
Facility reditel("Reditel");

Facility namestek_tech("Technicky namestek");
Facility namestek_ekon("Ekonomicky namestek");
Facility vedouci_tech("Vedouci technickeho useku");

Histogram delka("Doba zpracovani dokumentu", 0, 0.5, 50);


// stiznost zadost smlouva dotaz - delka zpracovani [hod]
#define STIZNOST 8
#define ZADOST 9
#define SMLOUVA 16
#define DOTAZ 8
#define OSTATNI 8


// TODO: poruchy skeneru jednou za 1000 hodin
// nebo systemu

// Prichozi dokument - od stiznosti, pres smlouvy, posudky, rozhodnuti...
class Dokument : public Process
{
    void Behavior()
    {
        born = Time;
        // sekretarka - prostuduje, naskenuje, oznaci, ulozi, zaradi
        // protridi, okomentuje, ...
        Seize(sekretarka);
        Wait(1.0 / 12);
        Release(sekretarka);

        // reditel velmi kratce prohledne dokument a prideli nekteremu useku
        Seize(reditel);
        Wait(1.0 / 80);
        Release(reditel);

        znova: 

        double usek = Random();
        if(usek < 0.33) { // usek reditele
            // dostava zadano primo podrizeny reditele, aby se tim zabyval
            // vlastne doruceno adresatovi, takze takovy dokument skoncil
            // TODO takto snadne?
            if(Random() < 0.3)
                goto znova;
        } else if(usek < 0.66) { // ekonomicky usek
            // TODO hm?
            Seize(namestek_ekon);
            Wait(1.0 / 10);
            Release(namestek_ekon);
            if(Random() < 0.3)
                goto znova;
        } else { // technicky usek
            Seize(namestek_tech);
            Wait(1.0 / 10);
            // u nej se to ma taky zdrzet den-dva
            Release(namestek_tech);
            if(Random() < 0.3)
                goto znova;
        }
        // TODO pred skokem zdrzet

        // TODO simulace spatne dorucenych papiru
        // TODO simulace nejakych smycek

        // TODO: dle typu papitu - udelat dobu zpracovani
        // TODO zivot jednoho papiru

        // TODO baba s postou?

        delka(Time - born);
    }
public:
    double born;
};

// Postacka dorucuje kazdy den postu - simulace prichodu posty
class Postacka : public Event
{
    void Behavior()
    {
        // pocet posty je rovnomerne rozlozen
        int p = Uniform(POST_AVR-POSR_VAR, POST_AVR+POSR_VAR);
        // postacka prijde rovnomerne v prubehu 1 hodiny
        double d = Random();
        // vsechny dopisy preda najednou
        for(int i = 0; i < p; ++i)
            (new Dokument())->Activate(Time + d);
        // postacka prijde i pristi den
        Activate(Time + WORKING_HOURS);
    }
};

// Proces cloveka
class Clovek : public Process
{
    void Behavior()
    {
        // prichozi clovek ma vyssi prioritu nez dokumenty sekretarky
        // ta dokonci svou praci a jde se mu venovat
        Priority = 1;
        // behem rozhovoru se sekretarkou vznikne dokument, ktery je treba
        // take vyridit
        Seize(sekretarka);
        Wait(1.0 / 6);
        (new Dokument())->Activate();
        Release(sekretarka);
        // spokojeny klient odchazi
    }
};

// Prichody lidi osobne
class Lide : public Event
{
    void Behavior()
    {
        (new Clovek())->Activate();
        Activate(Time + Exponential(8.0 / PEOPLE));
    }
};

class Email : public Event
{
    void Behavior()
    {
        // emailem prijde dokument
        (new Dokument())->Activate();
        Activate(Time + Exponential(8.0 / EMAIL));
    }
};


bool reditel_vpraci = false; // je reditel prave ted v praci?
int reditel_absence = 0; // kolik dnu po sobe neni v praci
class Reditel : public Process
{
    void Behavior()
    {
        // reditel se vetsinu casu venuje jinym povinnostem
        // hodinu denne se venuje administrative
        Priority = 1;
        while(1) {
            Seize(reditel, 1);
            reditel_vpraci = false;
            // 1,5 krat do tydne (3 * za 10 dnu) neni reditel k dispozici
            if(Random() < 0.3) {
                reditel_absence += 1;
                Wait(WORKING_HOURS-1);
                // pokud reditel nemuze 2 dny po sobe, tak ho zaskoci
                // technicky namestek
                if(reditel_absence > 1) {
                    Seize(namestek_tech, 1);
                    Release(reditel);
                    Wait(1);
                    Release(namestek_tech);
                } else {
                    Wait(1);
                    Release(reditel);
                }
            } else {
                Wait(WORKING_HOURS - 1);
                reditel_vpraci = true;
                reditel_absence = 0;
                Release(reditel);
                Wait(1);
            }
        }
    }
    // TODO: kdyz ma hodne prace, tak tomu dat vic casu
};
// TODO: zvazit zda je while(1) dobre nebo misto toho dat event, co se bude co
// den delat znova


// modelovani stridani dnu - jen pro ucely zobrazeni v grafu
bool day_odd = false;
class Day : public Event
{
    void Behavior()
    {
        day_odd = ! day_odd;
        Activate(Time + WORKING_HOURS);
    }
};


// sber casoveho prubehu delek front
void samp_s_f()
{
    Print("%lf %d %d %d %d %d\n",
        Time,
        sekretarka.QueueLen(),
        reditel.QueueLen(),
        reditel_vpraci ? 20 : 0,
        namestek_tech.QueueLen(),
        day_odd ? -5 : 0
    );
}
Sampler samp_s(samp_s_f, 1.0 / 10);




int main()
{
    // cas 0 je 8 rano prvniho pracovniho dne
    // n*WORKING_HOURS je 8 rano n-teho nasledujiciho dne
    // 1 odpovida 1 pracovni hodine (tj. cas volna a vikend se preskakuje)
    Init(0, DAYS * WORKING_HOURS);

    RandomSeed(time(NULL));

    // modelovani plynuti dnu (jen pro vizualizaci)
    (new Day())->Activate();

    // modelovani pracovni cinnosti reditele
    (new Reditel())->Activate();

    // postacka prichazi rano po 10. hodine (8+2)
    (new Postacka())->Activate(2);
    // generovani prichodu lidi
    (new Lide())->Activate();
    // generovani prichozich emailu
    (new Email())->Activate();

    // start simulace
    Run();

    // vypis statistik
    SetOutput("statistiky");
    sekretarka.Output();
    reditel.Output();
    delka.Output();

    return 0;
}
