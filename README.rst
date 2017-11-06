######
README
######

This describes a very simple file format for packing multiple files into
a single file, similar to tar, only simpler. It was designed simply to
demonstrate how multiple types of data can be packed into records in a
file.

The layout of the data in the file is described by the following table:

+========+=================+===========+=============================+
| Offset | Size (in bytes) | Type      | Meaning                     |
+========+=================+===========+=============================+
| 0      | 2               | Short     | Format Identifier           |
+--------+-----------------+-----------+-----------------------------+
| 2      | 2               | Short     | Format Version              |
+--------+-----------------+-----------+-----------------------------+
| 4      | 2               | Short     | Length of file name         |
+--------+-----------------+-----------+-----------------------------+
| 6      | LEN             | Char      | Filename of File Packed     |
+--------+-----------------+-----------+-----------------------------+
| 6+LEN  | 8               | long long | Size in bytes of file data  |
+--------+-----------------+-----------+-----------------------------+
| 14+LEN | FILE_SIZE       | Char      | File data                   |
+--------+-----------------+-----------+-----------------------------+


For example, lets assume the first file packed is named 'test.txt'. So it's
file name length is 8 bytes. Lets also assume that the file is a total of 392
bytes long. Given those numbers, the offsets would be:

0   - 2byte short Format identifier
2   - 2byte short format version number
4   - 2byte short length of filename
6   - 8 bytes, the characters in the filename
14  - 8 bytes long long int with the file size
22  - Start of the file data
414 - Start of next record (which is another 2byte short), or end of file
      if it's the last record in the file.

