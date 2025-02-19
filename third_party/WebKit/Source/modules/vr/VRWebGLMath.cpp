#include "VRWebGLMath.h"
#include <cassert>
#include <cmath>

void VRWebGL_multiplyMatrices4(const GLfloat* m, const GLfloat* n, GLfloat* o)
{
    if (o == m || o == n)
    {
        const GLfloat* ae = m;
        const GLfloat* be = n;
        GLfloat* te = o;

        GLfloat a11 = ae[ 0 ], a12 = ae[ 4 ], a13 = ae[ 8 ], a14 = ae[ 12 ];
        GLfloat a21 = ae[ 1 ], a22 = ae[ 5 ], a23 = ae[ 9 ], a24 = ae[ 13 ];
        GLfloat a31 = ae[ 2 ], a32 = ae[ 6 ], a33 = ae[ 10 ], a34 = ae[ 14 ];
        GLfloat a41 = ae[ 3 ], a42 = ae[ 7 ], a43 = ae[ 11 ], a44 = ae[ 15 ];

        GLfloat b11 = be[ 0 ], b12 = be[ 4 ], b13 = be[ 8 ], b14 = be[ 12 ];
        GLfloat b21 = be[ 1 ], b22 = be[ 5 ], b23 = be[ 9 ], b24 = be[ 13 ];
        GLfloat b31 = be[ 2 ], b32 = be[ 6 ], b33 = be[ 10 ], b34 = be[ 14 ];
        GLfloat b41 = be[ 3 ], b42 = be[ 7 ], b43 = be[ 11 ], b44 = be[ 15 ];

        te[ 0 ] = a11 * b11 + a12 * b21 + a13 * b31 + a14 * b41;
        te[ 4 ] = a11 * b12 + a12 * b22 + a13 * b32 + a14 * b42;
        te[ 8 ] = a11 * b13 + a12 * b23 + a13 * b33 + a14 * b43;
        te[ 12 ] = a11 * b14 + a12 * b24 + a13 * b34 + a14 * b44;

        te[ 1 ] = a21 * b11 + a22 * b21 + a23 * b31 + a24 * b41;
        te[ 5 ] = a21 * b12 + a22 * b22 + a23 * b32 + a24 * b42;
        te[ 9 ] = a21 * b13 + a22 * b23 + a23 * b33 + a24 * b43;
        te[ 13 ] = a21 * b14 + a22 * b24 + a23 * b34 + a24 * b44;

        te[ 2 ] = a31 * b11 + a32 * b21 + a33 * b31 + a34 * b41;
        te[ 6 ] = a31 * b12 + a32 * b22 + a33 * b32 + a34 * b42;
        te[ 10 ] = a31 * b13 + a32 * b23 + a33 * b33 + a34 * b43;
        te[ 14 ] = a31 * b14 + a32 * b24 + a33 * b34 + a34 * b44;

        te[ 3 ] = a41 * b11 + a42 * b21 + a43 * b31 + a44 * b41;
        te[ 7 ] = a41 * b12 + a42 * b22 + a43 * b32 + a44 * b42;
        te[ 11 ] = a41 * b13 + a42 * b23 + a43 * b33 + a44 * b43;
        te[ 15 ] = a41 * b14 + a42 * b24 + a43 * b34 + a44 * b44;
    }
    else 
    {
        o[0] = m[0]*n[0]  + m[4]*n[1]  + m[8]*n[2]  + m[12]*n[3];
        o[1] = m[1]*n[0]  + m[5]*n[1]  + m[9]*n[2]  + m[13]*n[3];
        o[2] = m[2]*n[0]  + m[6]*n[1]  + m[10]*n[2]  + m[14]*n[3];
        o[3] = m[3]*n[0]  + m[7]*n[1]  + m[11]*n[2]  + m[15]*n[3];
        o[4] = m[0]*n[4]  + m[4]*n[5]  + m[8]*n[6]  + m[12]*n[7];
        o[5] = m[1]*n[4]  + m[5]*n[5]  + m[9]*n[6]  + m[13]*n[7];
        o[6] = m[2]*n[4]  + m[6]*n[5]  + m[10]*n[6]  + m[14]*n[7];
        o[7] = m[3]*n[4]  + m[7]*n[5]  + m[11]*n[6]  + m[15]*n[7];
        o[8] = m[0]*n[8]  + m[4]*n[9]  + m[8]*n[10] + m[12]*n[11];
        o[9] = m[1]*n[8]  + m[5]*n[9]  + m[9]*n[10] + m[13]*n[11];
        o[10] = m[2]*n[8]  + m[6]*n[9]  + m[10]*n[10] + m[14]*n[11];
        o[11] = m[3]*n[8]  + m[7]*n[9]  + m[11]*n[10] + m[15]*n[11];
        o[12] = m[0]*n[12] + m[4]*n[13] + m[8]*n[14] + m[12]*n[15];
        o[13] = m[1]*n[12] + m[5]*n[13] + m[9]*n[14] + m[13]*n[15];
        o[14] = m[2]*n[12] + m[6]*n[13] + m[10]*n[14] + m[14]*n[15];
        o[15] = m[3]*n[12] + m[7]*n[13] + m[11]*n[14] + m[15]*n[15];
    }
}  

void VRWebGL_inverseMatrix4(const GLfloat* m, GLfloat* o)
{
    // based on http://www.euclideanspace.com/maths/algebra/matrix/functions/inverse/fourD/index.htm
    GLfloat* te = o;
    const GLfloat* me = m;

    GLfloat n11 = me[ 0 ], n21 = me[ 1 ], n31 = me[ 2 ], n41 = me[ 3 ],
	    n12 = me[ 4 ], n22 = me[ 5 ], n32 = me[ 6 ], n42 = me[ 7 ],
	    n13 = me[ 8 ], n23 = me[ 9 ], n33 = me[ 10 ], n43 = me[ 11 ],
	    n14 = me[ 12 ], n24 = me[ 13 ], n34 = me[ 14 ], n44 = me[ 15 ],

	    t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44,
	    t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44,
	    t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44,
	    t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

    GLfloat det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;

    assert(det != 0);

    GLfloat detInv = 1.0 / det;

    te[ 0 ] = t11 * detInv;
    te[ 1 ] = ( n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44 ) * detInv;
    te[ 2 ] = ( n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44 ) * detInv;
    te[ 3 ] = ( n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43 ) * detInv;

    te[ 4 ] = t12 * detInv;
    te[ 5 ] = ( n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44 ) * detInv;
    te[ 6 ] = ( n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44 ) * detInv;
    te[ 7 ] = ( n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43 ) * detInv;

    te[ 8 ] = t13 * detInv;
    te[ 9 ] = ( n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44 ) * detInv;
    te[ 10 ] = ( n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44 ) * detInv;
    te[ 11 ] = ( n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43 ) * detInv;

    te[ 12 ] = t14 * detInv;
    te[ 13 ] = ( n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34 ) * detInv;
    te[ 14 ] = ( n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34 ) * detInv;
    te[ 15 ] = ( n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33 ) * detInv;
}

void VRWebGL_transposeMatrix4(const GLfloat* m, GLfloat* o)
{
    if (o == m)
    {
        GLfloat t;
        t = m[1];
        o[1] = m[4];
        o[4] = t;
        t = m[2];
        o[2] = m[8];
        o[8] = t;
        t = m[3];
        o[3] = m[12];
        o[12] = t;
        t = m[6];
        o[6] = m[9];
        o[9] = t;
        t = m[7];
        o[7] = m[13];
        o[13] = t;
        t = m[11];
        o[11] = m[14];
        o[14] = t;
    }
    else
    {
        o[1] = m[4];
        o[4] = m[1];
        o[2] = m[8];
        o[8] = m[2];
        o[3] = m[12];
        o[12] = m[3];
        o[6] = m[9];
        o[9] = m[6];
        o[7] = m[13];
        o[13] = m[7];
        o[11] = m[14];
        o[14] = m[11];
    }
    o[0] = m[0];
    o[5] = m[5];
    o[10] = m[10];
    o[15] = m[15];
}

void VRWebGL_quaternionFromMatrix4(const GLfloat* m, GLfloat* o)
{
    assert(m != o);

    // Code from http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/

    // 2 dimensional matrix conversion:

    // FROM

    // 0,0  1,0  2,0
    // 0,1  1,1  2,1
    // 0,2  1,2  2,2

    // TO

    // 0    1    2
    // 4    5    6
    // 8    9    10

    GLfloat trace = m[0] + m[5] + m[10]; // I removed + 1.0f; see discussion with Ethan
    if( trace > 0 ) {// I changed M_EPSILON to 0
        GLfloat s = 0.5f / sqrtf(trace+ 1.0f);
        o[3] = 0.25f / s;
        o[0] = ( m[6] - m[9] ) * s;
        o[1] = ( m[8] - m[2] ) * s;
        o[2] = ( m[1] - m[4] ) * s;
    } else {
        if ( m[0] > m[5] && m[0] > m[10] ) {
            GLfloat s = 2.0f * sqrtf( 1.0f + m[0] - m[5] - m[10]);
            o[3] = (m[6] - m[9] ) / s;
            o[0] = 0.25f * s;
            o[1] = (m[4] + m[1] ) / s;
            o[2] = (m[8] + m[2] ) / s;
        } else if (m[5] > m[10]) {
            GLfloat s = 2.0f * sqrtf( 1.0f + m[5] - m[0] - m[10]);
            o[3] = (m[8] - m[2] ) / s;
            o[0] = (m[4] + m[1] ) / s;
            o[1] = 0.25f * s;
            o[2] = (m[9] + m[6] ) / s;
        } else {
            GLfloat s = 2.0f * sqrtf( 1.0f + m[10] - m[0] - m[5] );
            o[3] = (m[1] - m[4] ) / s;
            o[0] = (m[8] + m[2] ) / s;
            o[1] = (m[9] + m[6] ) / s;
            o[2] = 0.25f * s;
        }
    }

    // This algorithm does not seem to work properly.

    // GLfloat fTrace = m[0] + m[5] + m[10];
    // GLfloat fRoot;

    // if ( fTrace > 0.0 ) 
    // {
    //     // |w| > 1/2, may as well choose w > 1/2
    //     fRoot = sqrt(fTrace + 1.0);  // 2w
    //     o[3] = 0.5 * fRoot;
    //     fRoot = 0.5 / fRoot;  // 1/(4w)
    //     o[0] = (m[6] - m[9]) * fRoot;
    //     o[1] = (m[8] - m[2]) * fRoot;
    //     o[2] = (m[1] - m[4]) * fRoot;
    // } 
    // else 
    // {
    //     // |w| <= 1/2
    //     int i = 0;
    //     if ( m[5] > m[0] )
    //       i = 1;
    //     if ( m[10] > m[i*4+i] )
    //       i = 2;
    //     int j = (i+1)%4;
    //     int k = (i+2)%4;
        
    //     fRoot = sqrt(m[i*4+i]-m[j*4+j]-m[k*4+k] + 1.0);
    //     o[i] = 0.5 * fRoot;
    //     fRoot = 0.5 / fRoot;
    //     o[3] = (m[j*4+k] - m[k*4+j]) * fRoot;
    //     o[j] = (m[j*4+i] + m[i*4+j]) * fRoot;
    //     o[k] = (m[k*4+i] + m[i*4+k]) * fRoot;
    // }

};