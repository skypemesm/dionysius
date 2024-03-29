/* 
 * Copyright (c) 2006-2009 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * For licensing and other legal details, see the file zrtp_legal.c.
 * 
 * Viktor Krikun <v.krikun at zfoneproject.com> 
 */

/*
 * We want the copyright string accessable to the unix strings command in
 * the linked binary, and don't want the linker to remove it if it's not
 * referenced, thus the volatile qualifier.
 * 
 * ANSI C standard, section 3.5.3: "An object that has volatile-qualified
 * type may be modified in ways unknown to the implementation or have
 * other unknown side effects."
 */
extern volatile const char zrtpCopyright[];
