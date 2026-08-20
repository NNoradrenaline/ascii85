            a85x_encode(fp);
    } else {
        if (o.decode)
            ascii85_decode(fp, o.delims, o.ignore_garbage);
        else
            ascii85_encode(fp, o.delims, o.wrap, o.y_abbr);
    }

    if (fp != stdin && fclose(fp) != 0)
        die("input close error");
    if (fflush(stdout) == EOF)
        die("write error on standard output");

    return EXIT_SUCCESS;
}
