/*..ooOOO0000OOOoo......ooOOO0000OOOoo......ooOOO0000OOOoo...*\
 *
 * This is for to make a better benchmark for the Kallman filter 
 * using tha same system is implemented now in Gaudi. 
 * The code is copied and modifies for to run in an independient way out of gaudi.
 *
 \*...ooOOO0000OOOoo......ooOOO0000OOOoo......ooOOO0000OOOoo...*/

#ifndef BAD_H
#define BAD_H 1

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <vector>


//============ Covariance =============
class Covariance{
    public:
        Covariance(){};
        ~Covariance(){};
        const double& operator()(int i, int j) const{
            return m_cov[i][j];
            };
        double& operator()(int i, int j){
            return m_cov[i][j];
            };

    private:
        double m_cov[5][5]={};
    };

//======= End Covariance =========
//============ State =============
class State{
    public:
        State(){};
        ~State(){};
        Covariance covariance(){return m_covariance;}
        float x(){return m_x;}
        float y(){return m_y;}
        float z(){return m_z;}                
        float tx(){return m_tx;}
        float ty(){return m_ty;}        
        void setX(float x){m_x=x;}
        void setY(float x){m_y=x;}
        void setZ(float x){m_z=x;}
        void setTx(float x){m_tx=x;}
        void setTy(float x){m_ty=x;}        
        
    private:
        float m_x,m_y,m_z, m_tx, m_ty;
        Covariance m_covariance;
    };

//========= End State ============
//============ Hits ==============

class PrPixelHit {
    public:
        PrPixelHit()
            : m_x(0.),m_y(0.),m_z(0.),
              m_wxerr(0.),m_wyerr(0.){}
        
        PrPixelHit(const float x,const float y,const float z,
                   const float wxerr, const float wyerr)
            : m_x(x),m_y(y),m_z(z),
              m_wxerr(wxerr),m_wyerr(wyerr){}

        virtual ~PrPixelHit(){}

        void setHit(const float x,const float y,const float z,
                    const float wxerr, const float wyerr) {
            m_x = x;
            m_y = y;
            m_z = z;
            m_wxerr = wxerr;
            m_wyerr = wyerr;
            }

        float x() const { return m_x; }
        float y() const { return m_y; }
        float z() const { return m_z; }
        float wx() const { return m_wxerr * m_wxerr; }
        float wy() const { return m_wyerr * m_wyerr; }
        float wxerr() const { return m_wxerr; }
        float wyerr() const { return m_wyerr; }
        void print(){
            printf("%f %f %f %f %f\n",
                   m_x,m_y,m_z,m_wxerr,m_wyerr);
            }        
        
    private:
        float m_x,m_y,m_z,m_wxerr,m_wyerr;
    };
typedef std::vector<PrPixelHit> PrPixelHits;

//=========== End Hits ==============
//============ Tracks ===============

class PrPixelTrack {
    public:
        PrPixelTrack(unsigned int n=20)
            : m_tx(0.), m_ty(0.),
              m_x0(0.), m_y0(0.) {
            m_hits.reserve(n);
        }
        
        virtual ~PrPixelTrack(){}
        
        /// Return the list of hits on this track.
        PrPixelHits &hits() {
            return m_hits; }
        
        /// Add a given hit to this track
        void addHit(PrPixelHit &hit) {
            m_hits.push_back(hit); }

        // Number of hits assigned to the track
        unsigned int size(void) const { return m_hits.size(); }

        //Fit with a K-filter with scattering. Return the chi2
        float fitKalman(State &state, const int direction,
                        const float noisePerLayer) const;

    private:
        /// List of pointers to hits
        PrPixelHits m_hits;            
        /// Straight-line fit parameters
        float m_tx, m_ty, m_x0, m_y0;  

        friend class serializer;
    };
    /// vector of tracks
    typedef std::vector<PrPixelTrack> PrPixelTracks;  

//========== End Tracks ============
//============ Events ==============

class Event{
    public:
        Event(){
            m_tracks.reserve(200);}
        ~Event(){}
       
        PrPixelTracks &tracks() { return m_tracks;}
        void addTrack(PrPixelTrack &track){
            m_tracks.push_back(track); }
        unsigned int size(void) const {
            return m_tracks.size();}
    private:
        PrPixelTracks m_tracks;
    };
    /// vector of Events
    typedef std::vector<Event> Events;
    
//============ End Events ==============
//=============== Run ==================

class Run{
    public:
        Run(const char fn[]);
        ~Run(){};
        Events &events() { return m_events;}
        void addEvent(Event &event){
            m_events.push_back(event);
            }
        unsigned int size(void) const {
            return m_events.size();}        
        void print();
    private:
        FILE *fp;
        int nbevts, nbtracks, nbhits;
        Events m_events;
    };
//============= End Run ===============

#endif
