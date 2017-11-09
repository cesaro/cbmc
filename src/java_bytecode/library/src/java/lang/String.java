/*
 * Copyright (c) 1994, 2013, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

package java.lang;

import java.io.ObjectStreamField;
import java.io.UnsupportedEncodingException;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Locale;
import org.cprover.CProver;

public final class String
    implements java.io.Serializable, Comparable<String>, CharSequence {

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public String() {}

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public String(String original) {}

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public String(char value[]) {}

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public String(char value[], int offset, int count) {}

    public String(int[] codePoints, int offset, int count) {}

    @Deprecated
    public String(byte ascii[], int hibyte, int offset, int count) {}

    @Deprecated
    public String(byte ascii[], int hibyte) {}

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    private static void checkBounds(byte[] bytes, int offset, int length) {}

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public String(byte bytes[], int offset, int length, String charsetName)
            throws UnsupportedEncodingException {}

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public String(byte bytes[], int offset, int length, Charset charset) {}

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public String(byte bytes[], String charsetName)
            throws UnsupportedEncodingException {}

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public String(byte bytes[], Charset charset) {}

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public String(byte bytes[], int offset, int length) {}

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public String(byte bytes[]) {}

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public String(StringBuffer buffer) {}

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public String(StringBuilder builder) {}

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    String(char[] value, boolean share) {}

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public int length() {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public boolean isEmpty() {
        return CProver.nondetBoolean();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public char charAt(int index) {
        return CProver.nondetChar();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public int codePointAt(int index) {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public int codePointBefore(int index) {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public int codePointCount(int beginIndex, int endIndex) {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public int offsetByCodePoints(int index, int codePointOffset) {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public void getChars(int srcBegin, int srcEnd, char dst[], int dstBegin) {
    }

    @Deprecated
    public native void getBytes(int srcBegin, int srcEnd, byte dst[], int dstBegin);

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public byte[] getBytes(String charsetName)
        throws java.io.UnsupportedEncodingException {
        // DIFFBLUE MODEL LIBRARY this uses pointer equality for performances
        // This means this will only work when the argument corresponds to a
        // literal.
        if(charsetName == "US-ASCII")
            return getBytesAscii();

        if(charsetName == "UTF-16BE")
            return getBytesUTF_16BE();
        if(charsetName == "UTF-16LE")
            return getBytesUTF_16LE();
        if(charsetName == "UTF-16")
            return getBytesUTF_16();

        // For now these conversions are not efficient so we force the string
        // to be ASCII.
        if(charsetName == "ISO-8859-1")
            return getBytesEnforceAscii();
        if(charsetName == "UTF-8")
            return getBytesEnforceAscii();

        // DIFFBLUE MODEL LIBRARY @TODO: Support further encodings
        CProver.assume(charsetName.equals("US-ASCII"));
        return getBytesAscii();
        // Does not seem to work:
        // throw new java.io.UnsupportedEncodingException();
    }

    public byte[] getBytes(Charset charset) throws NullPointerException {
        // DIFFBLUE MODEL LIBRARY
        // @TODO: we always enforce ASCII for now because of a bug
        // with static object initialization, which makes all standard charsets
        // (US_ASCII, ISO_8859_1, ...) null.
        return getBytesEnforceAscii();
    }

    private byte[] getBytesAscii() {
        int l = length();
        byte result[] = new byte[l];
        for(int i = 0; i < l; i++)
        {
            char c = charAt(i);
            if(c>127)
                result[i] = (byte) '?';
            else
                result[i] = (byte) c;
        }
        return result;
    }

    // DIFFBLUE MODELS LIBRARY
    // This converts the String to a byte array and adds assumptions enforcing
    // all characters are valid ASCII
    private byte[] getBytesEnforceAscii() {
        int l = length();
        byte result[] = new byte[l];
        for(int i = 0; i < l; i++)
        {
            char c = charAt(i);
            CProver.assume(c<=127);
            result[i] = (byte) c;
        }
        return result;
    }

    private byte[] getBytesISO_8859_1() {
        int l = length();
        byte result[] = new byte[l];
        for(int i = 0; i < l; i++)
        {
            char c = charAt(i);
            if(c>255)
                result[i] = (byte) '?';
            else
                result[i] = (byte) c;
        }
        return result;
    }

    private byte[] getBytesUTF_16BE() {
        int l = length();
        byte result[] = new byte[2*l];
        for(int i = 0; i < l; i++)
        {
            char c = charAt(i);
            result[2*i] = (byte) (c >> 8);
            result[2*i+1] = (byte) (c & 0xFF);
        }
        return result;
    }

    private byte[] getBytesUTF_16LE() {
        int l = length();
        byte result[] = new byte[2*l];
        for(int i = 0; i < l; i++)
        {
            char c = charAt(i);
            result[2*i] = (byte) (c & 0xFF);
            result[2*i+1] = (byte) (c >> 8);
        }
        return result;
    }

    private byte[] getBytesUTF_16() {
        // Like UTF_16BE with FE FF at the beginning to mark byte order
        int l = length();
        byte result[] = new byte[2*l+2];
        result[0] = (byte) 0xFE;
        result[1] = (byte) 0xFF;
        for(int i = 2; i < l+2; i++)
        {
            char c = charAt(i);
            result[2*i] = (byte) (c >> 8);
            result[2*i+1] = (byte) (c & 0xFF);
        }
        return result;
    }

    private byte[] getBytesUTF_8() {
        int l = length();
        int output_size = 0;
        for(int i = 0; i < l; i++)
        {
            int c = charAt(i);
            if(c>=0xD800)
            {
                i++;
                c = 0x10000 | ((c & 0x3FF) << 10) | (charAt(i) & 0x3FF);
            }
            if(c<=0x7F)
                output_size += 1;
            else if(c<=0x7FF)
                output_size += 2;
            else if(c<=0xFFFF)
                output_size += 3;
            else
                output_size += 4;
        }

        byte result[] = new byte[output_size];
        int index = 0;
        for(int i = 0; i < l; i++)
        {
            int c = charAt(i);
            if(c>=0xD800)
            {
                i++;
                c = 0x10000 | ((c & 0x3FF) << 10) | (charAt(i) & 0x3FF);
            }
            if(c<=0x7F)
                result[index++]=(byte)c;
            else if(c<=0x7FF)
            {
                result[index++]=(byte)((c >> 6) | 0xC0);
                result[index++]=(byte)((c & 0x3F) | 0x80);
            }
            else if(c<=0xFFFF)
            {
                result[index++]=(byte)((c >> 12) | 0xE0);
                result[index++]=(byte)(((c >> 6) &0x3F) | 0x80);
                result[index++]=(byte)((c & 0x3F) | 0x80);
            }
            else
            {
                result[index++]=(byte)((c >> 18) | 0xF0);
                result[index++]=(byte)(((c >> 12) & 0x3F)| 0x80);
                result[index++]=(byte)(((c >> 6) & 0x3F) | 0x80);
                result[index++]=(byte)((c & 0x3F) | 0x80);
            }
        }
        return result;
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public byte[] getBytes() {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public boolean equals(Object anObject) {
        return CProver.nondetBoolean();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public boolean contentEquals(StringBuffer sb) {
        return CProver.nondetBoolean();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    private boolean nonSyncContentEquals(AbstractStringBuilder sb) {
        return CProver.nondetBoolean();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public boolean contentEquals(CharSequence cs) {
        return CProver.nondetBoolean();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public boolean equalsIgnoreCase(String anotherString) {
        return CProver.nondetBoolean();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public int compareTo(String anotherString) {
        return CProver.nondetInt();
    }

    public int compareToIgnoreCase(String str) {
        // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public boolean regionMatches(int toffset, String other, int ooffset,
            int len) {
        return CProver.nondetBoolean();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public boolean regionMatches(boolean ignoreCase, int toffset,
            String other, int ooffset, int len) {
        return CProver.nondetBoolean();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public boolean startsWith(String prefix, int toffset) {
        return CProver.nondetBoolean();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public boolean startsWith(String prefix) {
        return CProver.nondetBoolean();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public boolean endsWith(String suffix) {
        return CProver.nondetBoolean();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public int hashCode() {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public int indexOf(int ch) {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public int indexOf(int ch, int fromIndex) {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    private int indexOfSupplementary(int ch, int fromIndex) {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public int lastIndexOf(int ch) {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public int lastIndexOf(int ch, int fromIndex) {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    private int lastIndexOfSupplementary(int ch, int fromIndex) {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public int indexOf(String str) {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public int indexOf(String str, int fromIndex) {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    static int indexOf(char[] source, int sourceOffset, int sourceCount,
            String target, int fromIndex) {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    static int indexOf(char[] source, int sourceOffset, int sourceCount,
            char[] target, int targetOffset, int targetCount,
            int fromIndex) {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public int lastIndexOf(String str) {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public int lastIndexOf(String str, int fromIndex) {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    static int lastIndexOf(char[] source, int sourceOffset, int sourceCount,
            String target, int fromIndex) {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    static int lastIndexOf(char[] source, int sourceOffset, int sourceCount,
            char[] target, int targetOffset, int targetCount,
            int fromIndex) {
        return CProver.nondetInt();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public String substring(int beginIndex) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public String substring(int beginIndex, int endIndex) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public CharSequence subSequence(int beginIndex, int endIndex) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public String concat(String str) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public String replace(char oldChar, char newChar) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public boolean matches(String regex) {
        return CProver.nondetBoolean();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public boolean contains(CharSequence s) {
        return CProver.nondetBoolean();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public String replaceFirst(String regex, String replacement) {
        return CProver.nondetWithoutNull();
    }

    public String replaceAll(String regex, String replacement)
    {
        // DIFFBLUE MODELS LIBRARY: we assume the expression is just
        // a string literal
        CProver.assume(
            regex.indexOf('[') == -1 &&
            regex.indexOf(']') == -1 &&
            regex.indexOf('.') == -1 &&
            regex.indexOf('\\') == -1 &&
            regex.indexOf('?') == -1 &&
            regex.indexOf('^') == -1 &&
            regex.indexOf('$') == -1 &&
            regex.indexOf('*') == -1 &&
            regex.indexOf('+') == -1 &&
            regex.indexOf('{') == -1 &&
            regex.indexOf('}') == -1 &&
            regex.indexOf('|') == -1 &&
            regex.indexOf('(') == -1 &&
            regex.indexOf(')') == -1);
        return replace(regex, replacement);
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public String replace(CharSequence target, CharSequence replacement) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public String[] split(String regex, int limit) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public String[] split(String regex) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public static String join(CharSequence delimiter, CharSequence... elements) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public static String join(CharSequence delimiter,
            Iterable<? extends CharSequence> elements) {
        return CProver.nondetWithoutNull();
    }

    public String toLowerCase(Locale locale) {
        // DIFFBLUE MODEL LIBRARY We just ignore the argument as it does not
        // make a difference in most cases
        return toLowerCase();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public String toLowerCase() {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: call toUpperCase (as with toLowerCase(Locale)
    public String toUpperCase(Locale locale) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public String toUpperCase() {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public String trim() {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public String toString() {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public char[] toCharArray() {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY this method is treated internally in CBMC
    // The limitation of the current implementation are:
    //   - 'dateTime', 'hashcode' and 'octal' format specifiers are not 
    //     implemented
    //   - precision and width are ignored
    //   - the first argument of String.format should be a constant
    //   - we arbitrary limit the number of arguments to 10 (increasing 
    //     this number slows down the solver and when the number of 
    //     arguments exceeds the limit the result will be incorrect)
    //   - having 5 arguments or more makes the solver slow
    //   - a model of the objects given is required, and we don't provide
    //     such models for Float, and Character right now
    public static String format(String format, Object... args) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public static String format(Locale l, String format, Object... args) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public static String valueOf(Object obj) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public static String valueOf(char data[]) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public static String valueOf(char data[], int offset, int count) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public static String copyValueOf(char data[], int offset, int count) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public static String copyValueOf(char data[]) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public static String valueOf(boolean b) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public static String valueOf(char c) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public static String valueOf(int i) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public static String valueOf(long l) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY This is treated internally in CBMC
    public static String valueOf(float f) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public static String valueOf(double d) {
        return CProver.nondetWithoutNull();
    }

    // DIFFBLUE MODEL LIBRARY @TODO: implement this method internally in CBMC
    public String intern() {
        return CProver.nondetWithoutNull();
    }
}
