'' examples/manual/fileio/open.bas
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
'' See Also: http://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpen
'' --------

' Create a string and fill it.
Dim buffer As String, f As Integer
buffer = "Hello World within a file."

' Find the first free file number.
f = FreeFile

' Open the file "file.ext" for binary usage, using the file number "f".
Open "file.ext" For Binary As #f
If Err>0 Then Print "Error opening the file":End

' Place our string inside the file, using number "f".
Put #f, , buffer

' Close all open files.  
Close

' End the program. (Check the file "file.ext" upon running to see the output.)
End
