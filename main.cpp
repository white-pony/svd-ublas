#include <iostream>
#include <fstream>

#include <cmath>
#include <ctime>

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>


namespace ublas = boost::numeric::ublas;

#define DEBUG
#define CHECK_RESULT

void eye(ublas::matrix<float>& m) {
    for(unsigned int i = 0; i < m.size1(); i++)
        for(unsigned int j = 0; j < m.size2(); j++)
            m(i, j) = (i == j)?1.0f:0.0f;
}

float sign(float val) {
    return val >= 0.0f?1.0f:-1.0f;
}

float norm(ublas::vector<float>& x) {
    float x_norm = 0.0;
    for(unsigned int i = 0; i < x.size(); i++) x_norm += std::pow(x(i), 2);
    x_norm = std::sqrt(x_norm);
    return x_norm;
}

void normalize(ublas::vector<float>& x) {
    float x_norm = norm(x);
    for(unsigned int i = 0; i < x.size(); i++) {
        x(i) /= x_norm;
    }
}

void householder(ublas::matrix<float>& A,
                 ublas::matrix<float>& QQ,
                    unsigned int row_start,
                    unsigned int col_start,
                    bool column) {
    unsigned int size = column?A.size1():A.size2();
    unsigned int start = column?row_start:col_start;
    ublas::vector<float> x(size);
	for(unsigned int i = 0 ; i < size; i++) {
		if(i < start) {
			x(i) = 0;
		} else {
			if(column)
				x(i) = A(i, col_start);
			else
				x(i) = A(row_start, i);
		}
	}

    float x_norm = norm(x);
    float alpha = -sign(x(start)) * x_norm;

#ifdef DEBUG
    std::cout << "||x|| = " << x_norm << "\n";
    std::cout << "alpha = " << alpha << "\n";
#endif

    ublas::vector<float> v = x;

    v(start) += alpha;
    normalize(v);

#ifdef DEBUG
    std::cout << "v = " << v << "\n";
#endif



	ublas::matrix<float> Q;

#ifdef DEBUG
	Q.resize(size, size);
    eye(Q);

    for(unsigned int i = start; i < Q.size1(); i++) {
        for(unsigned int j = start; j < Q.size2(); j++) {
            Q(i, j) = Q(i, j) - 2 * v(i) * v(j);
        }
    }

    std::cout << "Q  = " << Q << "\n";
#endif

    if(column) {
        for(unsigned int i = 0; i < A.size2(); i++) {
            float sum_Av = 0.0f;

			for(unsigned int j = 0; j < A.size1(); j++) sum_Av = sum_Av + (v(j) * A(j, i));
            for(unsigned int j = 0; j < A.size1(); j++) A(j, i) = A(j, i) - 2 * v(j) * sum_Av;
        }

		for(unsigned int i = 0; i < A.size1(); i++) {
			float sum_Qv = 0.0f;

			for(unsigned int j = 0; j < A.size2(); j++) sum_Qv = sum_Qv + (v(j) * QQ(i, j));
            for(unsigned int j = 0; j < A.size2(); j++) QQ(i, j) = QQ(i, j) - 2 * v(j) * sum_Qv;
        }

    } else {
        for(unsigned int i = 0; i < A.size1(); i++) {
            float sum_Av = 0.0f;

			for(unsigned int j = 0; j < A.size2(); j++) sum_Av = sum_Av + (v(j) * A(i, j));
            for(unsigned int j = 0; j < A.size2(); j++) A(i, j) = A(i, j) - 2 * v(j) * sum_Av;
        }

		for(unsigned int i = 0; i < A.size2(); i++) {
			float sum_Qv = 0.0f;

            for(unsigned int j = 0; j < A.size1(); j++) sum_Qv = sum_Qv + (v(j) * QQ(j ,i));
            for(unsigned int j = 0; j < A.size1(); j++) QQ(j, i) = QQ(j, i) - 2 * v(j) * sum_Qv;
        }
    }
}

void bidiag(ublas::matrix<float>& A,
            ublas::matrix<float>& QQL,
            ublas::matrix<float>& QQR) {
    unsigned int row_num = A.size1();
	unsigned int col_num = A.size2();

    QQL.resize(row_num, row_num);
    QQR.resize(col_num, col_num);

    eye(QQL);
    eye(QQR);

	unsigned int to = std::min(row_num, col_num);
    for(unsigned int i = 0; i < to - 1; i++) {
        householder(A, QQL, i, i, true);
        if(i < to - 2) householder(A, QQR, i, i + 1, false);

#ifdef DEBUG
        std::cout << "QQL = " << QQL << "\n";
        std::cout << "QQR = " << QQR << "\n";
        std::cout << "AAA = " << A << "\n";
        std::cout << "*****************\n";
#endif
    }
}

void random_fill(ublas::matrix<float>& A, unsigned int size1, unsigned int size2) {
    A.resize(size1, size2);

    for(unsigned int i = 0; i < A.size1(); i++) {
        for(unsigned int j = 0; j < A.size2(); j++) {
            A(i, j) = (float)rand() / RAND_MAX;
        }
    }
}

float matrix_compare(ublas::matrix<float>& res, ublas::matrix<float>& ref) {
    float diff = 0.0;

    for(unsigned int i = 0; i < res.size1(); i++) {
        for(unsigned int j = 0; j < res.size2(); j++) {
            diff = std::max(diff, std::abs(res(i, j) - ref(i,j)));
        }
    }

    return diff;
}

bool check_bidiag(ublas::matrix<float>& A) {
    const float EPS = 0.0001f;

    for(unsigned int i = 0; i < A.size1(); i++) {
        for(unsigned int j = 0; j < A.size2(); j++) {
            if((std::abs(A(i, j)) > EPS) && (i != j) && ((i + 1) != j)) {
                std::cout << "Failed at " << i << " " << j << " " << A(i, j) << "\n";
                return false;
            }
        }
    }
    return true;
}

int main() {
    srand((unsigned int)time(0));

    ublas::matrix<float> in;

    std::fstream f;
    f.open("data/pysvd.example", std::fstream::in);
    f >> in;
    f.close();

//    random_fill(in, 1024);

    ublas::matrix<float> ref = in;
#ifdef DEBUG
    std::cout << in << "\n";
#endif
    ublas::matrix<float> QQL;
    ublas::matrix<float> QQR;

    bidiag(in, QQL, QQR);

    ublas::matrix<float> result;

#ifdef CHECK_RESULT
    result = ublas::prod(in, QQR);
    result = ublas::prod(QQL, result);

    //std::cout << result << "\n";
#endif

	std::cout << "DIFF    = " << matrix_compare(result, ref) << "\n";
    std::cout << "Is bidiag " << check_bidiag(in) << "\n";
	return 0;
}
