/*
 * IMS projekt - Workflow procesu v administrative
 *
 * Cupak Michal xcupak04
 * Dujicek Ales xdujic01
 */

#include <simlib.h>
#include <ctime>

// vyber nastaveni modelu
#define SIM_MODEL_VAK
//#define SIM_EXPERIMENT1
//#define SIM_EXPERIMENT2
//#define SIM_EXPERIMENT3
//#define SIM_EXPERIMENT4


// model dle zjistenych dat
#if defined(SIM_MODEL_VAK)

    // pracovni doba [hod] (7.5)
    #define WORKING_HOURS 7.5
    // delka simulace [dny]
    #define DAYS 50

    // pocet prichozi posty denne [kus] (25-35)
    #define POST_COUNT Uniform(20, 30)
    // doba mezi prichody lidi [hod] (10 denne)
    #define PEOPLE_INTERVAL Exponential(WORKING_HOURS / 10)
    // doba mezi prichody elektronickych zprav [hod] (20 denne)
    #define EMAIL_INTERVAL Exponential(WORKING_HOURS / 20)
    // doba obsluhy lidi [hod] (prum. 5 min.)
    #define PEOPLE_SER Exponential(1.0/12)
    // doba mezi poruchami skeneru [hod] (1-2 za mesic)
    #define PORUCHA (WORKING_HOURS * 10)

    // pravdepodobnost, ze reditel neni dany den v praci (30%)
    #define OOO_DIRECTOR 0.3

    // zpracovani dokumentu sekretarkou [hod] (5-7 min)
    #define WOR_SEC Uniform(1.0/12, 1.0/8.5)
    #define WOR_DIR (1.0 / 80)
    #define WOR_TCH_DEP
    #define WOR_EKO_DEP
    #define WOR

    // procentualni rozdeleni dokumentu mezi useky
    #define SEC_DIRECTOR 0.05
    #define SEC_ECO 0.45
    #define SEC_TECH 0.50

    // stiznost zadost smlouva dotaz - delka zpracovani [dny]
    #define STIZNOST 5-10
    #define ZADOST 10
    #define SMLOUVA 15
    #define DOTAZ 5-10

// experimenty ...
#elif defined(SIM_EXPERIMENT1)

#elif defined(SIM_EXPERIMENT2)

#elif defined(SIM_EXPERIMENT3)

#elif defined(SIM_EXPERIMENT4)

#else
    #error "Vyberte nektery z preddefinovanych modelu"
#endif


Facility sekretarka("Sekretarka");
Facility reditel("Reditel");

Facility namestek_tech("Technicky namestek");
Facility namestek_ekon("Ekonomicky namestek");
Facility vedouci_tech("Vedouci technickeho useku");

Histogram delka("Doba zpracovani dokumentu", 0, 0.5, 50);

extern int posta;

// Prichozi dokument - od stiznosti, pres smlouvy, posudky, rozhodnuti...
class Dokument : public Process
{
    void Behavior()
    {
        born = Time;
        // sekretarka - prostuduje, naskenuje, oznaci, ulozi, zaradi
        // protridi, okomentuje, ...
        Seize(sekretarka);
        Wait(WOR_SEC);
        Release(sekretarka);

        // reditel velmi kratce prohledne dokument a prideli nekteremu useku
        Seize(reditel);
        Wait(WOR_DIR);
        Release(reditel);

        if(0) {
            // dokument dorucen spatnemu cloveku, trochu se tim zdrzi
            dorucen_zle:
            Wait(1); // TODO ???
        }

        double usek = Random();
        if(usek < SEC_DIRECTOR) { // usek reditele
            // dostava zadano primo podrizeny reditele, aby se tim zabyval
            // vlastne doruceno adresatovi, takze takovy dokument skoncil
            if(Random() < 0.02)
                goto dorucen_zle;
        } else if(usek < SEC_DIRECTOR + SEC_ECO) { // ekonomicky usek
            Seize(namestek_ekon);
            Wait(1.0 / 10);
            Release(namestek_ekon);
            if(Random() < 0.02)
                goto dorucen_zle;
        } else { // technicky usek
            Seize(namestek_tech);
            Wait(1.0 / 10);
            // u nej se to ma taky zdrzet den-dva
            Release(namestek_tech);
            if(Random() < 0.02)
                goto dorucen_zle;
        }


        // na polovinu dokumentu se odpovida
        if(Random() < 0.5) {
            posta += 1;
        }

        delka(Time - born);
    }
public:
    double born;
};


// odvoz posty
class OdvozPosty : public Event
{
    void Behavior()
    {
        // postacka si odnese odchozi postu
        posta = 0;
    }
};


int posta = 0; // pocet posty k odeslani
// Postacka dorucuje kazdy den postu - simulace prichodu posty
class Postacka : public Event
{
    void Behavior()
    {
        // pocet posty je rovnomerne rozlozen
        int p = POST_COUNT;
        // postacka prijde rovnomerne v prubehu 1 hodiny
        double d = Random();
        // vsechny dopisy preda najednou
        for(int i = 0; i < p; ++i)
            (new Dokument())->Activate(Time + d);
        // postacka si odnese odchozi postu
        (new OdvozPosty())->Activate(Time + d);
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
        Wait(PEOPLE_SER);
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
        Activate(Time + PEOPLE_INTERVAL);
    }
};

// Prichody elektronickych dokumentu (email + datove schanky)
class Email : public Event
{
    void Behavior()
    {
        // emailem prijde dokument
        (new Dokument())->Activate();
        Activate(Time + EMAIL_INTERVAL);
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
            if(Random() < OOO_DIRECTOR) {
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

bool porucha = false;
class Porucha : public Process
{
    void Behavior()
    {
        // porucha se objevuje 1-2 x za mesic
        while(1) {
            double d = Exponential(WORKING_HOURS*40/3.0);
            Wait(d);
            Seize(sekretarka, 1);
            porucha = true;
            Wait(3);
            Release(sekretarka);
            porucha = false;
        }
    }
};


// sber casoveho prubehu delek front
void samp_s_f()
{
    Print("%lf %d %d %d %d %d %d %d\n",
        Time,
        sekretarka.QueueLen(),
        reditel.QueueLen(),
        reditel_vpraci ? -10 : 0,
        namestek_tech.QueueLen(),
        day_odd ? -5 : 0,
        porucha ? -10 : 0,
        posta
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

    // poruchy zarizeni sekretarky
    (new Porucha())->Activate();

    // start simulace
    Run();

    // vypis statistik
    SetOutput("statistiky");
    sekretarka.Output();
    reditel.Output();
    delka.Output();

    return 0;
}
