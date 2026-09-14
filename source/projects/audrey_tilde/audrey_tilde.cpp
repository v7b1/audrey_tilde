
// audrey~ a feedback synth engine
//
// porting Audrey_II by Synthux Academy to MaxMSP
// vboehm, 2026


#include "c74_msp.h"
#include "FeedbackSynthEngine.h"


using namespace daisysp;
using namespace c74::max;


static t_class* myObj_class = nullptr;



#define BLOCKSIZE 8

// original samplerate and blocksize
//static const auto kSampleRate = SaiHandle::Config::SampleRate::SAI_48KHZ;
//static const size_t kBlockSize = 4;


typedef struct _myObj {
    t_pxobject    x_obj;
    infrasonic::FeedbackSynth::Engine *engine;
    Limiter limiter[2];
    double  sr;

    float   string_pitch, string_pitch_target;
    float   fb_gain, fb_gain_target;
    float   reverb_mix, reverb_mix_target;
    float   hpf_, lpf_, hpf_target, lpf_target;
    float   echo_time, echo_scalar;
    short   pitch_connected;
} t_myObj;


void myObj_float(t_myObj *self, double freq);
void *myObj_new(t_symbol *s, long argc, t_atom *argv);

void myObj_dsp64(t_myObj *self, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void myObj_assist(t_myObj *self, void *b, long m, long a, char *s);
void myObj_free(t_myObj *self);



void *myObj_new(t_symbol *s, long argc, t_atom *argv)
{
    t_myObj* self = (t_myObj*)object_alloc(myObj_class);
    dsp_setup((t_pxobject *)self, 2);
    outlet_new((t_pxobject *)self, "signal");
    outlet_new((t_pxobject *)self, "signal");

    self->sr = sys_getsr();
    self->echo_scalar = 0.5f;
    self->echo_time = 1.0f;
    
    self->engine = new infrasonic::FeedbackSynth::Engine;
    self->engine->Init(self->sr);
    
    self->limiter[0].Init();
    self->limiter[1].Init();
    
    self->string_pitch = self->string_pitch_target = 40.f;
    self->hpf_ = self->hpf_target = 100.f;
    self->lpf_ = self->lpf_target = 8000.f;
    self->fb_gain_target = -18.0f;
    self->reverb_mix_target = 0.1;
    
    self->engine->SetOutputLevel(1.0f);
    
    // check arguments:
    if(argc>=1) {
        if(atom_gettype(argv)== A_LONG) {
            long n = atom_getlong(argv);
            self->string_pitch_target = DSY_CLAMP(n, -120., 120.0);
        }
        else if(atom_gettype(argv)== A_FLOAT) {
            double n = atom_getlong(argv);
            self->string_pitch_target = DSY_CLAMP(n, -120., 120.0);
        }
        else {
            object_error((t_object *)self, "bad argument, initializing pitch to 40.0");
        }
    }

    
    return (self);
}


void myObj_float(t_myObj *self, double n) {
    long innum = proxy_getinlet((t_object *)self);
    
    switch (innum) {
        case 0:
            //
            break;
        case 1:
            self->string_pitch_target = DSY_CLAMP(n, -120., 120.0);
            break;
    }
}

void myObj_int(t_myObj *self, long n) {
    long innum = proxy_getinlet((t_object *)self);
    switch (innum) {
        case 0:
            //
            break;
        case 1:
            self->string_pitch_target = DSY_CLAMP(n, -120., 120.0);
            break;
    }
}


// feedback (body) controls
void myObj_fb_gain(t_myObj *self, double f) {
    // initial value: -60.0f, min: -60.0f, max: 12.0f,
    float gain = (f * 72.0) - 60.0; // expects 0..1,
    gain = DSY_CLAMP(gain, -60., 12.);
    self->fb_gain_target = gain;
//    self->engine->SetFeedbackGain(gain);
}

void myObj_fb_delay(t_myObj *self, double f) {
    // inivial: 0.001f, min: 0.001f, max: 0.1f
    f *= 0.25f;         // expects 0..1, scales to 0..0.25
    self->engine->SetFeedbackDelay(f);
}



// filter stuff
void myObj_filter(t_myObj *self, double hp, double lp) {
    // --> log mapping...
    // initial: 250.0f, 10.0f, 4000.0f,
    self->hpf_target = fmap(hp, 10.0, 4000.0, Mapping::LOG);
//    self->engine->SetFeedbackHPFCutoff(hpf);
    
    // initial: 18000.0f, min: 100.0f, max: 18000.0f,
    self->lpf_target = fmap(lp, 100.0, 18000.0, Mapping::LOG);
//    self->engine->SetFeedbackLPFCutoff(lpf);
    
}



// reverb stuff
void myObj_reverb(t_myObj *self, double mix, double decay) {
    
    float mixf = DSY_CLAMP(mix, 0., 1.);
    self->reverb_mix_target = mixf * mixf;
//    self->engine->SetReverbMix(mix);
    
    // initial: 0.2f, min: 0.2f, max: 1.0f,
    float decayf = infrasonic::ftension(decay, -3.0f);
    decayf = fmap(decayf, 0.2f, 1.0f);
    self->engine->SetReverbFeedback(decayf);
}

void myObj_drive(t_myObj *self, double drive) {
    float d = DSY_CLAMP(drive, 0.01, 0.999);
    self->engine->SetDriveAmount(d);
}


// echo stuff
void myObj_echo(t_myObj *self, double amount, double time, double fb) {
    
    // initial: 0.0f, min: 0.0f, max: 1.0f, exponetial
    float a = fmap(amount, 0., 1.0, Mapping::EXP);
    self->engine->SetEchoDelaySendAmount(a);
    
    // initial: 0.5f, min: 0.05f, max: 5.0f
    self->echo_time = fmap(time, 0.05, 5.0, Mapping::EXP);
    self->engine->SetEchoDelayTime(self->echo_time * self->echo_scalar);
    
    float fbf = fmap(fb, 0., 1.5);
    self->engine->SetEchoDelayFeedback(fbf);
}

void myObj_echo_scale(t_myObj *self, long a) {
    if (a != 0) self->echo_scalar = 1.0f;
    else self->echo_scalar = 0.5f;
    self->engine->SetEchoDelayTime(self->echo_time * self->echo_scalar);
}



void myObj_perform64(t_myObj *self, t_object *dsp64, double **ins,
                     long numins, double **outs, long numouts,
                     long sampleframes, long flags, void *userparam)
{
    t_double    *in1 = ins[0];          // audio input
    t_double    *in2 = ins[1];          // string pitch
    t_double    *out1 = outs[0];
    t_double    *out2 = outs[1];

    long vs = sampleframes;
    infrasonic::FeedbackSynth::Engine *engine = self->engine;
    
    float interpol_coef = 100.f * BLOCKSIZE / self->sr;
    
    float string_pitch_target = self->string_pitch_target;
    float string_pitch = self->string_pitch;
    float fb_gain = self->fb_gain;
    float fb_gain_target = self->fb_gain_target;
    float reverb_mix = self->reverb_mix;
    float reverb_mix_target = self->reverb_mix_target;
    float hpf = self->hpf_;
    float hpf_target = self->hpf_target;
    float lpf = self->lpf_;
    float lpf_target = self->lpf_target;
    
    
    
    for (size_t i=0; i<vs; i+=BLOCKSIZE)
    {
        // smooth params
        if (self->pitch_connected) {
            string_pitch = in2[i];
        } else {
            fonepole(string_pitch, string_pitch_target, interpol_coef * 0.1f);
        }
        engine->SetStringPitch(string_pitch);
        
        fonepole(fb_gain, fb_gain_target, interpol_coef);
        engine->SetFeedbackGain(fb_gain);
        
        fonepole(hpf, hpf_target, interpol_coef);
        engine->SetFeedbackHPFCutoff(hpf);
        fonepole(lpf, lpf_target, interpol_coef);
        engine->SetFeedbackLPFCutoff(lpf);
        
        fonepole(reverb_mix, reverb_mix_target, interpol_coef * 0.2f);
        engine->SetReverbMix(reverb_mix);
        
        
        for (size_t k=0; k<BLOCKSIZE; k++)
        {
            uint8_t idx = i + k;
            float outL, outR;
            engine->Process(in1[idx], outL, outR);
            
            out1[idx] = (double)outL;
            out2[idx] = (double)outR;
        }
        
    }
    
    self->limiter[0].ProcessBlock(out1, vs, 0.7);
    self->limiter[1].ProcessBlock(out2, vs, 0.7);
    
    self->fb_gain = fb_gain;
    self->string_pitch = string_pitch;
    self->reverb_mix = reverb_mix;
    self->hpf_ = hpf;
    self->lpf_ = lpf;
}


void myObj_dsp64(t_myObj *self, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    self->sr = samplerate;
    self->pitch_connected = count[1];

    if (maxvectorsize >= BLOCKSIZE)
        object_method(dsp64, gensym("dsp_add64"), self, myObj_perform64, 0, NULL);
    else
        object_error((t_object *)self, "signal vector size can't be smaller than %d", BLOCKSIZE);
}




void myObj_free(t_myObj *self) {
    dsp_free((t_pxobject *)self);
    delete self->engine;
}


void myObj_assist(t_myObj *self, void *b, long m, long a, char *s)
{
    if (m == ASSIST_INLET) {
        switch (a) {
            case 0:
                std::strncpy(s,"(Signal) audio input", ASSIST_STRING_MAXSIZE);
                break;
            case 1:
                std::strncpy(s,"(Signal/float) pitch", ASSIST_STRING_MAXSIZE);
                break;

        }
    }
    else {
        switch (a) {
            case 0:
                std::strncpy(s,"(Signal) outL", ASSIST_STRING_MAXSIZE);
                break;
            case 1:
                std::strncpy(s,"(Signal) outR", ASSIST_STRING_MAXSIZE);
                break;
            
        }
    }
}



void ext_main(void *r)
{
    t_class *c = class_new("audrey~", (method)myObj_new, (method)myObj_free, sizeof(t_myObj), NULL, A_GIMME, 0);
    
    class_addmethod(c, (method)myObj_dsp64, "dsp64",    A_CANT, 0);
    class_addmethod(c, (method)myObj_assist, "assist",  A_CANT, 0);
    class_addmethod(c, (method)myObj_float, "float",  A_FLOAT, 0);
    class_addmethod(c, (method)myObj_int, "int",  A_LONG, 0);
    class_addmethod(c, (method)myObj_fb_gain, "fb_gain",  A_FLOAT, 0);
    class_addmethod(c, (method)myObj_fb_delay, "delay",  A_FLOAT, 0);
    class_addmethod(c, (method)myObj_drive, "drive",  A_FLOAT, 0);
    class_addmethod(c, (method)myObj_filter, "filter",  A_FLOAT, A_FLOAT, 0);
    class_addmethod(c, (method)myObj_reverb, "reverb",  A_FLOAT, A_FLOAT, 0);
    class_addmethod(c, (method)myObj_echo, "echo",  A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(c, (method)myObj_echo_scale, "double",  A_LONG, 0);
    class_dspinit(c);
    
    class_register(CLASS_BOX, c);
    myObj_class = c;

    
    object_post(NULL, "audrey~ based on 'Audrey II' by Synthux Academy");
}
