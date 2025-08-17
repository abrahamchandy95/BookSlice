BookSlice converts a PDF with a Table of Contents into structured chapter sections on disk and ingests them into MongoDB.

After cloning, edit include/db/mongo_config.hpp and set uri/db/coll to match your machine (e.g., mongodb://localhost:27017).

Build/run with your Makefile: make run (builds and runs) and make clean (cleans). Or use CMake directly if you prefer. You can also run the full pipeline with: ./run.sh /path/to/YourBook.pdf

Outputs go to chapters/, toc_sections/, and chapter_segments/, and sections are upserted into MongoDB using a unique key (book_title, chapter, title).

Development reset: mongosh -> use bookslice -> db.dropDatabase()

If the PDF has no usable TOC the pipeline exits early.
