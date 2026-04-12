#include <bits/stdc++.h>

using namespace std;

unsigned char LED_Value1 = 1;
unsigned char LED_Value2 = 128;
unsigned char LED_Value = 0;

void DisplayLEDs()
{
    // if (mode == 0)
    // {
    //     LED_Value = 0;
    // }
    LED_Value1 = (LED_Value1 >> 1) | (LED_Value1 << 7);
    LED_Value2 = (LED_Value2 << 1) | (LED_Value2 >> 7);
    LED_Value = LED_Value1 | LED_Value2;
    cout << bitset<8>(LED_Value) << endl;
}

int main()
{

    while (1)
    {
        DisplayLEDs();
        sleep(1);
    }

    return 0;
}