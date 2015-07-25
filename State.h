#ifndef STATE_H_
#define STATE_H_ 1

using namespace std;
//============ Covariance =============
class Covariance{
    public:
        Covariance(){};
        ~Covariance(){};
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
        Covariance& covariance(){return m_covariance;}
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
    typedef std::vector<State> States;

//========= End State ============

#endif
