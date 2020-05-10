#pragma once
#pragma warning(disable:4334)
#pragma warning(disable:4244)
#pragma warning(disable:4800)
 
#include <Windows.h>
#include <stdint.h>
 
class CKeyEvents
{
private:
	HHOOK hhookObject;
 
	uint64_t Segments [ 4 ];
 
	void SetBitToZero( uint64_t &Segment, uint8_t Shifting ) const { Segment &= ~( 1 << Shifting ); }
 
	void SetBitToOne( uint64_t &Segment, uint8_t Shifting ) const { Segment |= 1 << Shifting; }
 
	uint64_t GetBitFromSegment( uint64_t &Segment, uint8_t Shifting ) const { return Segment & 1 << Shifting; }
 
	void SetKeyState( uint8_t Key, uint8_t State )
	{
		uint8_t SegmentID = Key / 64;
 
		uint8_t Shifting = Key - SegmentID * 64;
 
		!State ? SetBitToZero( Segments[ SegmentID ], Shifting ) : SetBitToOne( Segments [ SegmentID ], Shifting );
	}
 
	static LRESULT CALLBACK KeyboardHook( int nCode, WPARAM wParam, LPARAM lParam )
	{
		if ( nCode < HC_ACTION ) { return CallNextHookEx( nullptr, nCode, wParam, lParam ); }
 
		PKBDLLHOOKSTRUCT Events = reinterpret_cast< PKBDLLHOOKSTRUCT >( lParam );
 
		if ( wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN )
		{
			GetSingleton( )->SetKeyState( Events->vkCode, 1 );
		}
		
		if ( wParam == WM_KEYUP || wParam == WM_SYSKEYUP )
		{
			GetSingleton( )->SetKeyState( Events->vkCode, 0 );
		}
 
		return CallNextHookEx( nullptr, nCode, wParam, lParam );
	}
 
	CKeyEvents( )
	{
		hhookObject = nullptr;
		
		memset( Segments, 0, sizeof( Segments ) );
	}
 
	~CKeyEvents( )
	{
		
	}
 
public:
	static CKeyEvents* GetSingleton( )
	{
		static CKeyEvents Singleton;
 
		return &Singleton;
	}
	
	bool GetKeyState( uint8_t Key ) 
	{
		uint8_t SegmentID = Key / 64;
 
		uint8_t Shifting = Key - SegmentID * 64;
 
		return GetBitFromSegment( Segments[ SegmentID ], Shifting );
	}
 
	bool Hook( )
	{
		hhookObject = SetWindowsHookEx( WH_KEYBOARD_LL, KeyboardHook, nullptr, NULL );
 
		return hhookObject != nullptr;
	}
 
	bool Unhook( )
	{
		memset( Segments, 0, sizeof( Segments ) );
 
		return UnhookWindowsHookEx( hhookObject );
	}
};