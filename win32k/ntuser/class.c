/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/class.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
/* INCLUDES ******************************************************************/

#include <w32k.h>

//#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
InitClassImpl(VOID)
{
   return(STATUS_SUCCESS);
}

NTSTATUS FASTCALL
CleanupClassImpl(VOID)
{
   return(STATUS_SUCCESS);
}

VOID FASTCALL
ClassReferenceClass(PWNDCLASS_OBJECT Class)
{
   //Class->RefCount++;      
}

VOID FASTCALL
UserDereferenceClass(PWNDCLASS_OBJECT Class)
{
   //if (--Class->RefCount == 0)
   //{
   //   
   //}
}

BOOL FASTCALL
ClassReferenceClassByAtom(
   PWNDCLASS_OBJECT* Class,
   RTL_ATOM Atom,
   HINSTANCE hInstance)
{
   PWNDCLASS_OBJECT Current, BestMatch = NULL;
   PLIST_ENTRY CurrentEntry;
   PW32PROCESS Process = PsGetWin32Process();

   CurrentEntry = Process->ClassListHead.Flink;
   while (CurrentEntry != &Process->ClassListHead)
   {
      Current = CONTAINING_RECORD(CurrentEntry, WNDCLASS_OBJECT, ListEntry);

      if (Current->Atom == Atom && (hInstance == NULL || Current->hInstance == hInstance))
      {
         *Class = Current;
         ClassReferenceClass(Current);
         //         IntUnLockProcessClasses(Process);
         return TRUE;
      }

      if (Current->Atom == Atom && Current->Global)
         BestMatch = Current;

      CurrentEntry = CurrentEntry->Flink;
   }

   if (BestMatch != NULL)
   {
      *Class = BestMatch;
      //ObmReferenceObject(BestMatch);
      ClassReferenceClass(BestMatch);
      return TRUE;
   }

   return FALSE;
}

BOOL FASTCALL
ClassReferenceClassByName(
   PWNDCLASS_OBJECT *Class,
   LPCWSTR ClassName,
   HINSTANCE hInstance)
{
   PWINSTATION_OBJECT WinStaObject;
   NTSTATUS Status;
   BOOL Found;
   RTL_ATOM ClassAtom;

   if (!ClassName || !PsGetWin32Thread()->Desktop)
      return FALSE;

   WinStaObject = UserGetCurrentWinSta();

   Status = RtlLookupAtomInAtomTable(
               WinStaObject->AtomTable,
               (LPWSTR)ClassName,
               &ClassAtom);

   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Failed to lookup class atom!\n");
      return FALSE;
   }

   Found = ClassReferenceClassByAtom(Class, ClassAtom, hInstance);

   return Found;
}

BOOL FASTCALL
ClassReferenceClassByNameOrAtom(
   PWNDCLASS_OBJECT *Class,
   LPCWSTR ClassNameOrAtom,
   HINSTANCE hInstance)
{
   BOOL Found;

   if (IS_ATOM(ClassNameOrAtom))
      Found = ClassReferenceClassByAtom(Class, (RTL_ATOM)((ULONG_PTR)ClassNameOrAtom), hInstance);
   else
      Found = ClassReferenceClassByName(Class, ClassNameOrAtom, hInstance);

   return Found;
}

DWORD STDCALL
NtUserGetClassInfo(
   HINSTANCE hInstance,
   LPCWSTR lpClassName,
   LPWNDCLASSEXW lpWndClassEx,
   BOOL Ansi,
   DWORD unknown3)
{
   PWNDCLASS_OBJECT Class;
   RTL_ATOM Atom;
   DECLARE_RETURN(DWORD);

   if (IS_ATOM(lpClassName))
   {
      DPRINT("Enter NtUserGetClassInfo - %x (%lx)\n", lpClassName, hInstance);
   }
   else
   {
      DPRINT("Enter NtUserGetClassInfo - %S (%lx)\n", lpClassName, hInstance);
   }

   UserEnterExclusive();

   if (!ClassReferenceClassByNameOrAtom(&Class, lpClassName, hInstance))
   {
      SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
      RETURN(0);
   }

   lpWndClassEx->cbSize = sizeof(WNDCLASSEXW);
   lpWndClassEx->style = Class->style;
   if (Ansi)
      lpWndClassEx->lpfnWndProc = Class->lpfnWndProcA;
   else
      lpWndClassEx->lpfnWndProc = Class->lpfnWndProcW;
   lpWndClassEx->cbClsExtra = Class->cbClsExtra;
   lpWndClassEx->cbWndExtra = Class->cbWndExtra;
   /* This is not typo, we're really not going to use Class->hInstance here. */
   lpWndClassEx->hInstance = hInstance;
   lpWndClassEx->hIcon = Class->hIcon;
   lpWndClassEx->hCursor = Class->hCursor;
   lpWndClassEx->hbrBackground = Class->hbrBackground;
   if (Class->lpszMenuName.MaximumLength)
      RtlCopyUnicodeString((PUNICODE_STRING)lpWndClassEx->lpszMenuName, &Class->lpszMenuName);
   else
      lpWndClassEx->lpszMenuName = Class->lpszMenuName.Buffer;
   lpWndClassEx->lpszClassName = lpClassName;
   lpWndClassEx->hIconSm = Class->hIconSm;
   Atom = Class->Atom;

   //ObmDereferenceObject(Class);
   ClassReferenceClass(Class);

   RETURN(Atom);

CLEANUP:
   DPRINT("Leave NtUserGetClassInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

ULONG FASTCALL
IntGetClassName(struct _WINDOW_OBJECT *WindowObject, LPWSTR lpClassName,
                ULONG nMaxCount)
{
   ULONG Length;
   LPWSTR Name;
   PWINSTATION_OBJECT WinStaObject;
   NTSTATUS Status;

   if(!PsGetWin32Thread()->Desktop)
   {
      return 0;
   }

   WinStaObject = UserGetCurrentWinSta();

   Length = 0;
   Status = RtlQueryAtomInAtomTable(WinStaObject->AtomTable,
                                    WindowObject->Class->Atom, NULL, NULL, NULL, &Length);
   Name = ExAllocatePoolWithTag(PagedPool, Length + sizeof(UNICODE_NULL), TAG_STRING);
   Status = RtlQueryAtomInAtomTable(WinStaObject->AtomTable,
                                    WindowObject->Class->Atom, NULL, NULL, Name, &Length);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("IntGetClassName: RtlQueryAtomInAtomTable failed\n");
      return 0;
   }
   Length /= sizeof(WCHAR);
   if (Length > nMaxCount)
   {
      Length = nMaxCount;
   }
   wcsncpy(lpClassName, Name, Length);
   /* FIXME: Check buffer size before doing this! */
   *(lpClassName + Length) = 0;
   ExFreePool(Name);

   return Length;
}

DWORD STDCALL
NtUserGetClassName (
   HWND hWnd,
   LPWSTR lpClassName,
   ULONG nMaxCount)
{
   PWINDOW_OBJECT WindowObject;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetClassName\n");
   UserEnterExclusive();

   WindowObject = IntGetWindowObject(hWnd);
   if (WindowObject == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(0);
   }

   RETURN(IntGetClassName(WindowObject, lpClassName, nMaxCount));

CLEANUP:
   DPRINT("Leave NtUserGetClassName, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

DWORD STDCALL
NtUserGetWOWClass(DWORD Unknown0,
                  DWORD Unknown1)
{
   UNIMPLEMENTED;
   return(0);
}

PWNDCLASS_OBJECT FASTCALL ClassCreateClass(DWORD bytes)
{
   PWNDCLASS_OBJECT class = ExAllocatePool(PagedPool, bytes);
   return class;
}


PWNDCLASS_OBJECT FASTCALL
IntCreateClass(
   CONST WNDCLASSEXW *lpwcx,
   DWORD Flags,
   WNDPROC wpExtra,
   PUNICODE_STRING MenuName,
   RTL_ATOM Atom)
{
   PWNDCLASS_OBJECT ClassObject;
   BOOL Global;

   Global = (Flags & REGISTERCLASS_SYSTEM) || (lpwcx->style & CS_GLOBALCLASS) ? TRUE : FALSE;

   /* Check for double registration of the class. */
   if (PsGetWin32Process() != NULL)
   {
      if (ClassReferenceClassByAtom(&ClassObject, Atom, lpwcx->hInstance))
      {
         /*
          * NOTE: We may also get a global class from
                               * ClassReferenceClassByAtom. This simple check
                               * prevents that we fail valid request.
                               */
         if (ClassObject->hInstance == lpwcx->hInstance)
         {
            SetLastWin32Error(ERROR_CLASS_ALREADY_EXISTS);
            UserDereferenceClass(ClassObject);
            return(NULL);
         }
      }
   }

   ClassObject = ClassCreateClass(sizeof(WNDCLASS_OBJECT) + lpwcx->cbClsExtra);
   if (ClassObject == NULL)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return(NULL);
   }

   ClassObject->cbSize = lpwcx->cbSize;
   ClassObject->style = lpwcx->style;
   ClassObject->cbClsExtra = lpwcx->cbClsExtra;
   ClassObject->cbWndExtra = lpwcx->cbWndExtra;
   ClassObject->hInstance = lpwcx->hInstance;
   ClassObject->hIcon = lpwcx->hIcon;
   ClassObject->hCursor = lpwcx->hCursor;
   ClassObject->hbrBackground = lpwcx->hbrBackground;
   ClassObject->Unicode = !(Flags & REGISTERCLASS_ANSI);
   ClassObject->Global = Global;
   ClassObject->hIconSm = lpwcx->hIconSm;
   ClassObject->Atom = Atom;
   if (wpExtra == NULL)
   {
      if (Flags & REGISTERCLASS_ANSI)
      {
         ClassObject->lpfnWndProcA = lpwcx->lpfnWndProc;
         ClassObject->lpfnWndProcW = (WNDPROC)IntAddWndProcHandle(lpwcx->lpfnWndProc,FALSE);
      }
      else
      {
         ClassObject->lpfnWndProcW = lpwcx->lpfnWndProc;
         ClassObject->lpfnWndProcA = (WNDPROC)IntAddWndProcHandle(lpwcx->lpfnWndProc,TRUE);
      }
   }
   else
   {
      if (Flags & REGISTERCLASS_ANSI)
      {
         ClassObject->lpfnWndProcA = lpwcx->lpfnWndProc;
         ClassObject->lpfnWndProcW = wpExtra;
      }
      else
      {
         ClassObject->lpfnWndProcW = lpwcx->lpfnWndProc;
         ClassObject->lpfnWndProcA = wpExtra;
      }
   }
   if (MenuName->Length == 0)
   {
      ClassObject->lpszMenuName.Length =
         ClassObject->lpszMenuName.MaximumLength = 0;
      ClassObject->lpszMenuName.Buffer = MenuName->Buffer;
   }
   else
   {
      ClassObject->lpszMenuName.Length =
         ClassObject->lpszMenuName.MaximumLength = MenuName->MaximumLength;
      ClassObject->lpszMenuName.Buffer = ExAllocatePoolWithTag(PagedPool, ClassObject->lpszMenuName.MaximumLength, TAG_STRING);
      RtlCopyUnicodeString(&ClassObject->lpszMenuName, MenuName);
   }
   /* Extra class data */
   if (ClassObject->cbClsExtra != 0)
   {
      ClassObject->ExtraData = (PCHAR)(ClassObject + 1);
      RtlZeroMemory(ClassObject->ExtraData, (ULONG)ClassObject->cbClsExtra);
   }
   else
   {
      ClassObject->ExtraData = NULL;
   }

   InitializeListHead(&ClassObject->ClassWindowsListHead);

   return(ClassObject);
}

RTL_ATOM STDCALL
NtUserRegisterClassExWOW(
   CONST WNDCLASSEXW* lpwcx,
   PUNICODE_STRING ClassName,
   PUNICODE_STRING ClassNameCopy,
   PUNICODE_STRING MenuName,
   WNDPROC wpExtra, /* FIXME: Windows uses this parameter for something different. */
   DWORD Flags,
   DWORD Unknown7)

/*
 * FUNCTION:
 *   Registers a new class with the window manager
 * ARGUMENTS:
 *   lpwcx          = Win32 extended window class structure
 *   bUnicodeClass = Whether to send ANSI or unicode strings
 *                   to window procedures
 *   wpExtra       = Extra window procedure, if this is not null, its used for the second window procedure for standard controls.
 * RETURNS:
 *   Atom identifying the new class
 */
{
   WNDCLASSEXW SafeClass;
   PWINSTATION_OBJECT WinStaObject;
   PWNDCLASS_OBJECT ClassObject;
   NTSTATUS Status;
   RTL_ATOM Atom;
   DECLARE_RETURN(RTL_ATOM);

   DPRINT("Enter NtUserRegisterClassExWOW\n");
   UserEnterExclusive();
   CHECKPOINT;
   if (!lpwcx)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN((RTL_ATOM)0);
   }

   if (Flags & ~REGISTERCLASS_ALL)
   {
      SetLastWin32Error(ERROR_INVALID_FLAGS);
      RETURN((RTL_ATOM)0);
   }

   Status = MmCopyFromCaller(&SafeClass, lpwcx, sizeof(WNDCLASSEXW));
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN((RTL_ATOM)0);
   }

   /* Deny negative sizes */
   if (lpwcx->cbClsExtra < 0 || lpwcx->cbWndExtra < 0)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN((RTL_ATOM)0);
   }

   WinStaObject = UserGetCurrentWinSta();

   if (ClassName->Length > 0)
   {
      DPRINT("NtUserRegisterClassExWOW(%S)\n", ClassName->Buffer);
      /* FIXME - Safely copy/verify the buffer first!!! */
      Status = RtlAddAtomToAtomTable(WinStaObject->AtomTable,
                                     ClassName->Buffer,
                                     &Atom);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Failed adding class name (%S) to atom table\n",
                 ClassName->Buffer);
         SetLastNtError(Status);
         RETURN((RTL_ATOM)0);
      }
   }
   else
   {
      Atom = (RTL_ATOM)(ULONG)ClassName->Buffer;
   }

   ClassObject = IntCreateClass(&SafeClass, Flags, wpExtra, MenuName, Atom);

   if (ClassObject == NULL)
   {
      if (ClassName->Length)
      {
         RtlDeleteAtomFromAtomTable(WinStaObject->AtomTable, Atom);
      }
      DPRINT("Failed creating window class object\n");
      RETURN((RTL_ATOM)0);
   }

   InsertTailList(&PsGetWin32Process()->ClassListHead, &ClassObject->ListEntry);
   RETURN(Atom);

CLEANUP:
   DPRINT("Leave NtUserRegisterClassExWOW, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

ULONG FASTCALL
UserGetClassLong(PWINDOW_OBJECT WindowObject, ULONG Offset, BOOL Ansi)
{
   LONG Ret;

   if ((int)Offset >= 0)
   {
      DPRINT("GetClassLong(%x, %d)\n", WindowObject->Self, Offset);
      if ((Offset + sizeof(LONG)) > WindowObject->Class->cbClsExtra)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return 0;
      }
      Ret = *((LONG *)(WindowObject->Class->ExtraData + Offset));
      DPRINT("Result: %x\n", Ret);
      return Ret;
   }

   switch (Offset)
   {
      case GCL_CBWNDEXTRA:
         Ret = WindowObject->Class->cbWndExtra;
         break;
      case GCL_CBCLSEXTRA:
         Ret = WindowObject->Class->cbClsExtra;
         break;
      case GCL_HBRBACKGROUND:
         Ret = (ULONG)WindowObject->Class->hbrBackground;
         break;
      case GCL_HCURSOR:
         Ret = (ULONG)WindowObject->Class->hCursor;
         break;
      case GCL_HICON:
         Ret = (ULONG)WindowObject->Class->hIcon;
         break;
      case GCL_HICONSM:
         Ret = (ULONG)WindowObject->Class->hIconSm;
         break;
      case GCL_HMODULE:
         Ret = (ULONG)WindowObject->Class->hInstance;
         break;
      case GCL_MENUNAME:
         Ret = (ULONG)WindowObject->Class->lpszMenuName.Buffer;
         break;
      case GCL_STYLE:
         Ret = WindowObject->Class->style;
         break;
      case GCL_WNDPROC:
         if (Ansi)
         {
            Ret = (ULONG)WindowObject->Class->lpfnWndProcA;
         }
         else
         {
            Ret = (ULONG)WindowObject->Class->lpfnWndProcW;
         }
         break;
      default:
         Ret = 0;
         break;
   }
   return(Ret);
}

DWORD STDCALL
NtUserGetClassLong(HWND hWnd, DWORD Offset, BOOL Ansi)
{
   PWINDOW_OBJECT WindowObject;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetClassLong\n");
   UserEnterExclusive();

   WindowObject = IntGetWindowObject(hWnd);
   if (WindowObject == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(0);
   }
   RETURN(UserGetClassLong(WindowObject, Offset, Ansi));

CLEANUP:
   DPRINT("Leave NtUserGetClassLong, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

void FASTCALL
IntSetClassLong(PWINDOW_OBJECT WindowObject, ULONG Offset, LONG dwNewLong, BOOL Ansi)
{
   PWINDOW_OBJECT Parent, Owner;

   if ((int)Offset >= 0)
   {
      DPRINT("SetClassLong(%x, %d, %x)\n", WindowObject->Self, Offset, dwNewLong);
      if ((Offset + sizeof(LONG)) > WindowObject->Class->cbClsExtra)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return;
      }
      *((LONG *)(WindowObject->Class->ExtraData + Offset)) = dwNewLong;
      return;
   }

   switch (Offset)
   {
      case GCL_CBWNDEXTRA:
         WindowObject->Class->cbWndExtra = dwNewLong;
         break;
      case GCL_CBCLSEXTRA:
         WindowObject->Class->cbClsExtra = dwNewLong;
         break;
      case GCL_HBRBACKGROUND:
         WindowObject->Class->hbrBackground = (HBRUSH)dwNewLong;
         break;
      case GCL_HCURSOR:
         WindowObject->Class->hCursor = (HCURSOR)dwNewLong;
         break;
      case GCL_HICON:
         WindowObject->Class->hIcon = (HICON)dwNewLong;
         Owner = IntGetOwner(WindowObject);
         Parent = UserGetParent(WindowObject);

         if ((!Owner) && (!Parent))
         {
            IntShellHookNotify(HSHELL_REDRAW, (LPARAM) WindowObject->Self);
         }

         break;
      case GCL_HICONSM:
         WindowObject->Class->hIconSm = (HICON)dwNewLong;
         break;
      case GCL_HMODULE:
         WindowObject->Class->hInstance = (HINSTANCE)dwNewLong;
         break;
      case GCL_MENUNAME:
         if (WindowObject->Class->lpszMenuName.MaximumLength)
            RtlFreeUnicodeString(&WindowObject->Class->lpszMenuName);
         if (!IS_INTRESOURCE(dwNewLong))
         {
            WindowObject->Class->lpszMenuName.Length =
               WindowObject->Class->lpszMenuName.MaximumLength = ((PUNICODE_STRING)dwNewLong)->MaximumLength;
            WindowObject->Class->lpszMenuName.Buffer = ExAllocatePoolWithTag(PagedPool, WindowObject->Class->lpszMenuName.MaximumLength, TAG_STRING);
            RtlCopyUnicodeString(&WindowObject->Class->lpszMenuName, (PUNICODE_STRING)dwNewLong);
         }
         else
         {
            WindowObject->Class->lpszMenuName.Length =
               WindowObject->Class->lpszMenuName.MaximumLength = 0;
            WindowObject->Class->lpszMenuName.Buffer = (LPWSTR)dwNewLong;
         }
         break;
      case GCL_STYLE:
         WindowObject->Class->style = dwNewLong;
         break;
      case GCL_WNDPROC:
         if (Ansi)
         {
            WindowObject->Class->lpfnWndProcA = (WNDPROC)dwNewLong;
            WindowObject->Class->lpfnWndProcW = (WNDPROC) IntAddWndProcHandle((WNDPROC)dwNewLong,FALSE);
            WindowObject->Class->Unicode = FALSE;
         }
         else
         {
            WindowObject->Class->lpfnWndProcW = (WNDPROC)dwNewLong;
            WindowObject->Class->lpfnWndProcA = (WNDPROC) IntAddWndProcHandle((WNDPROC)dwNewLong,TRUE);
            WindowObject->Class->Unicode = TRUE;
         }
         break;
   }
}

DWORD STDCALL
NtUserSetClassLong(HWND hWnd,
                   DWORD Offset,
                   LONG dwNewLong,
                   BOOL Ansi)
{
   PWINDOW_OBJECT WindowObject;
   LONG Ret;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserSetClassLong\n");
   UserEnterExclusive();

   WindowObject = IntGetWindowObject(hWnd);
   if (WindowObject == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN( 0);
   }
   Ret = UserGetClassLong(WindowObject, Offset, Ansi);
   IntSetClassLong(WindowObject, Offset, dwNewLong, Ansi);
   RETURN(Ret);

CLEANUP:
   DPRINT("Leave NtUserSetClassLong, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

DWORD STDCALL
NtUserSetClassWord(DWORD Unknown0,
                   DWORD Unknown1,
                   DWORD Unknown2)
{
   UNIMPLEMENTED;
   return(0);
}

BOOL STDCALL
NtUserUnregisterClass(
   LPCWSTR ClassNameOrAtom,
   HINSTANCE hInstance,
   DWORD Unknown)
{
   PWNDCLASS_OBJECT Class;
   PWINSTATION_OBJECT WinStaObject;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserUnregisterClass(%S)\n", ClassNameOrAtom);
   UserEnterExclusive();

   if (!ClassNameOrAtom || !PsGetWin32Thread()->Desktop)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN(FALSE);
   }

   WinStaObject = UserGetCurrentWinSta();

   if (!ClassReferenceClassByNameOrAtom(&Class, ClassNameOrAtom, hInstance))
   {
      SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
      RETURN(FALSE);
   }

   if (Class->hInstance && Class->hInstance != hInstance)
   {
      UserDereferenceClass(Class);
      SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
      RETURN(FALSE);
   }

   if (!IsListEmpty(&Class->ClassWindowsListHead))
   {
      /* Dereference the ClassReferenceClassByNameOrAtom() call */
      UserDereferenceClass(Class);
      SetLastWin32Error(ERROR_CLASS_HAS_WINDOWS);
      RETURN(FALSE);
   }

   /* Dereference the ClassReferenceClassByNameOrAtom() call */
   UserDereferenceClass(Class);

   RemoveEntryList(&Class->ListEntry);

   RtlDeleteAtomFromAtomTable(WinStaObject->AtomTable, Class->Atom);

   /* Free the object */
   UserDereferenceClass(Class);

   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserUnregisterClass, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/* EOF */
