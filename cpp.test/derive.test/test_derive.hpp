
class Base {
    public:
        Base(int i) : m_baseI(i), m_value(i)
        {
            printf("Base::Base()\n");
        };
        virtual void Print()
        {
            printf("Base::Print()\n");
        };
        virtual void SetI()
        {
            printf("Base::SetI()\n");
        }
        void GetI()
        {
            printf("Base::GetI(%d)\n", m_baseI);
        };
        virtual ~Base()
        {
            printf("Base::~Base()\n");
        };
        int getValue(){printf("value:%d\n",  m_value);};
    protected:
        int m_value;
    private:
        int m_baseI;
};

class Derive : public  Base {
    public:
        Derive(int j) : Base(1000), m_DeriveI(j)
        {
            printf("Derive::Derive()\n");
        };
        virtual void Print()
        {
            printf("Derive::Print()\n");
        };
        virtual void Derive_Print()
        {
            printf("Derive::Derive_Print()\n");
        };
        void test()
        {
            printf("Derive::test()\n");
        };
        virtual void hhh()
        {
            printf("hhh\n");
        };
        virtual void ggg()
        {
            printf("ggg\n");
        };
        virtual ~ Derive()
        {
            printf("Derive::~Derive()\n");
        };
    private:
        int m_DeriveI;
};
