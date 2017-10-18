# IF3130 - Sliding Window Protocol
---
### Prerequisites
Sebelum menjalankan program ini, pastikan Anda memiliki:
 - [GNU Make](https://www.gnu.org/software/make/)
 - [g++](https://gcc.gnu.org/)

### Installing 
Untuk menginstall program ini, lakukan dengan:
```sh
git clone https://github.com/keychera/slidingwindow.git
cd slidingwindow
make
```

### Running
- For Receiving File: `./recvile <filename> <windowsize> <buffersize> <port>`
- For Sending File: `./sendfile <filename> <windowsize> <buffersize> <destination_ip> <destination_port>`

---
## Sliding Window Protocol
Cara Kerja Sliding Window Setelah client mengirimkan data, server akan menerimanya dan memberikan respon berupa Acknowledgment(ACK) untuk setiap paket yang telah diterimanya. Data ACK yang diterima oleh client berisi sequence number untuk data berikutnya serta ukuran window saat ini. Kemudian client akan menggeser kepala windows ke data dengan sequence number sesuai pada ACK. Diantara proses client mengirimkan data dan server menerima terdapat kasus-kasus tertentu, agar tidak terjadi error dan flow control yang baik.


### Jawaban

1.  Penanganan Advertised Window Bernilai 0 Data sewajarnya tidak akan diproses. Window size dengan lebar 0 mengindikasikan bahwa banyaknya segmen yang diambil setiap kali proses pengiriman berlangsung adalah 0. Untuk mengatasi masalah tersebut, dapat dibuat kasus khusus agar ketika pengguna memasukkan window size 0, maka terdapat default number agar data tetap dapat dikirim dengan menggunakan algoritma sliding window.

2. Sliding Window
* Source Port (2 bytes): Port sender
* Destination Port (2 bytes): Port receiver
* Sequence Number: 4 bytes yang bertindak sebagai penanda urutan dari pengiriman paket pesan.
* Acknowledgment Number: Merupakan segment yang dibutuhkan oleh segmen TCP dengan flag ACK diset bernilai 1, dan mengindikasi oktet dalam byte yang diharapkan diterima oleh pengirim dari pengirim di pengiriman berikutnya.
* Data Offset: 4 bit yang menunjukkan size dari TCP Header dalam 32 bit words. Mengindikasi di offset berapa data dimulai dan panjang dari TCP Header merupakan kelipatan 32 bit.
* Reserved: Untuk keperluan ke depannya dan harus diset dengan nilai nol.
* Control Flags (< 10 bits): bertindak sebagai penanda aliran data untuk setiap situasi yang terjadi.
* Window Size : Berisi size dari receive window
* Checksum : Berisi 16 bit yang digenerate dari kumpulan field diatas dan digunakan untuk error-checking
* Urgent Pointer : Bila URG flag diset, maka field ini berisi offset dari sequence number yang menunjuk ke last byte dari urgent data
* Options : Size dari option ditentukan oleh data offset dan memiliki hingga 3 buah field yaitu Option-Kind, Option-Data, dan Option-Length
### Authors
 - **Kevin Erdiza**      13515016   | [keychera](https://github.com/keychera)
    *Fungsi: -*
 - **Taufan Mahaputra** 13515028    | [taufanmahaputra](https://github.com/taufanmahaputra)
    *Fungsi: -*
 - **Nicholas Thie**    13515079    | [leechopper15](https://github.com/leechopper15)
    *Fungsi: -*

# License
----

MIT
